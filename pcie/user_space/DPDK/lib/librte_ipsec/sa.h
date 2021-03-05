/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2018 Intel Corporation
 */

#ifndef _SA_H_
#define _SA_H_

#include <rte_rwlock.h>

#define IPSEC_MAX_HDR_SIZE	64
#define IPSEC_MAX_IV_SIZE	16
#define IPSEC_MAX_IV_QWORD	(IPSEC_MAX_IV_SIZE / sizeof(uint64_t))

/* padding alignment for different algorithms */
enum {
	IPSEC_PAD_DEFAULT = 4,
	IPSEC_PAD_AES_CBC = IPSEC_MAX_IV_SIZE,
	IPSEC_PAD_AES_GCM = IPSEC_PAD_DEFAULT,
	IPSEC_PAD_NULL = IPSEC_PAD_DEFAULT,
};

/* these definitions probably has to be in rte_crypto_sym.h */
union sym_op_ofslen {
	uint64_t raw;
	struct {
		uint32_t offset;
		uint32_t length;
	};
};

union sym_op_data {
#ifdef __SIZEOF_INT128__
	__uint128_t raw;
#endif
	struct {
		uint8_t *va;
		rte_iova_t pa;
	};
};

#define REPLAY_SQN_NUM		2
#define REPLAY_SQN_NEXT(n)	((n) ^ 1)

struct replay_sqn {
	rte_rwlock_t rwl;
	uint64_t sqn;
	__extension__ uint64_t window[0];
};

struct rte_ipsec_sa {
	uint64_t type;     /* type of given SA */
	uint64_t udata;    /* user defined */
	uint32_t size;     /* size of given sa object */
	uint32_t spi;
	/* sqn calculations related */
	uint64_t sqn_mask;
	struct {
		uint32_t win_sz;
		uint16_t nb_bucket;
		uint16_t bucket_index_mask;
	} replay;
	/* template for crypto op fields */
	struct {
		union sym_op_ofslen cipher;
		union sym_op_ofslen auth;
	} ctp;
	uint32_t salt;
	uint8_t proto;    /* next proto */
	uint8_t aad_len;
	uint8_t hdr_len;
	uint8_t hdr_l3_off;
	uint8_t icv_len;
	uint8_t sqh_len;
	uint8_t iv_ofs; /* offset for algo-specific IV inside crypto op */
	uint8_t iv_len;
	uint8_t pad_align;

	/* template for tunnel header */
	uint8_t hdr[IPSEC_MAX_HDR_SIZE];

	/*
	 * sqn and replay window
	 * In case of SA handled by multiple threads *sqn* cacheline
	 * could be shared by multiple cores.
	 * To minimise perfomance impact, we try to locate in a separate
	 * place from other frequently accesed data.
	 */
	union {
		union {
			rte_atomic64_t atom;
			uint64_t raw;
		} outb;
		struct {
			uint32_t rdidx; /* read index */
			uint32_t wridx; /* write index */
			struct replay_sqn *rsn[REPLAY_SQN_NUM];
		} inb;
	} sqn;

} __rte_cache_aligned;

int
ipsec_sa_pkt_func_select(const struct rte_ipsec_session *ss,
	const struct rte_ipsec_sa *sa, struct rte_ipsec_sa_pkt_func *pf);

#endif /* _SA_H_ */
