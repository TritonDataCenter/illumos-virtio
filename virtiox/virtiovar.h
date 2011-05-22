/*	$NetBSD$	*/

/*
 * Copyright (c) 2010 Minoura Makoto.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Part of the file derived from `Virtio PCI Card Specification v0.8.6 DRAFT'
 * Appendix A.
 */
/* An interface for efficient virtio implementation.
 *
 * This header is BSD licensed so anyone can use the definitions
 * to implement compatible drivers/servers.
 *
 * Copyright 2007, 2009, IBM Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifndef _DEV_PCI_VIRTIOVAR_H_
#define	_DEV_PCI_VIRTIOVAR_H_

#include <sys/types.h>
#include <sys/dditypes.h>
#include <sys/cmn_err.h>
#include <sys/list.h>

#define TRACE { \
	cmn_err (CE_NOTE, "^%s:%d %s()\n", __FILE__, __LINE__, __func__); \
	/* delay(drv_usectohz(1000000)); */\
}

#define FAST_TRACE { \
	cmn_err (CE_NOTE, "^%s:%d %s()\n", __FILE__, __LINE__, __func__); \
}
typedef boolean_t bool;
#define __packed  __attribute__((packed))

struct vq_entry {
	list_node_t		qe_list;
	struct virtqueue	*qe_queue;
	uint16_t		qe_index; /* index in vq_desc array */
	uint16_t		qe_used_len; /* Set when the descriptor gets back from device*/
	/* followings are used only when it is the `head' entry */
	struct vq_entry		*qe_next;
	ddi_dma_handle_t	qe_dmah;
	struct vring_desc	*qe_desc;
};

struct virtqueue {
	struct virtiox_softc	*vq_owner;
        unsigned int		vq_num; /* queue size (# of entries) */
	int			vq_index; /* queue number (0, 1, ...) */

	/* vring pointers (KVA) */
        struct vring_desc	*vq_descs;
        struct vring_avail	*vq_avail;
        struct vring_used	*vq_used;
	void			*vq_indirect;

	/* virtqueue allocation info */
	void			*vq_vaddr;
	int			vq_availoffset;
	int			vq_usedoffset;
	int			vq_indirectoffset;
	ddi_dma_cookie_t	vq_dma_cookie;
	ddi_dma_handle_t	vq_dma_handle;
	ddi_acc_handle_t	vq_dma_acch;

	int			vq_maxsegsize;
	int			vq_maxnsegs;

	/* free entry management */
	struct vq_entry		*vq_entries;
	list_t			vq_freelist;
	kmutex_t		vq_freelist_lock;

	/* enqueue/dequeue status */
	uint16_t		vq_avail_idx;
	uint16_t		vq_used_idx;
	int			vq_queued;
};

struct virtiox_softc {
	dev_info_t		*sc_dev;

	ddi_iblock_cookie_t     sc_icookie;

	ddi_acc_handle_t	sc_ioh;
	uint8_t			*sc_io_addr;
	int			sc_config_offset;

	uint32_t		sc_features;

	int			sc_nvqs; /* set by the user */ 
	struct virtqueue	*sc_vqs; /* set by the user */
};

/* The standard layout for the ring is a continuous chunk of memory which
 * looks like this.  We assume num is a power of 2.
 *
 * struct vring {
 *      // The actual descriptors (16 bytes each)
 *      struct vring_desc desc[num];
 *
 *      // A ring of available descriptor heads with free-running index.
 *      __u16 avail_flags;
 *      __u16 avail_idx;
 *      __u16 available[num];
 *
 *      // Padding to the next align boundary.
 *      char pad[];
 *
 *      // A ring of used descriptor heads with free-running index.
 *      __u16 used_flags;
 *      __u16 used_idx;
 *      struct vring_used_elem used[num];
 * };
 * Note: for virtio PCI, align is 4096.
 */

/* public interface */
uint32_t virtiox_negotiate_features(struct virtiox_softc*, uint32_t);
void virtiox_set_status(struct virtiox_softc *sc, int );
#define virtiox_device_reset(sc)	virtiox_set_status((sc), 0)

uint8_t virtiox_read_device_config_1(struct virtiox_softc *, int);
uint16_t virtiox_read_device_config_2(struct virtiox_softc *, int);
uint32_t virtiox_read_device_config_4(struct virtiox_softc *, int);
uint64_t virtiox_read_device_config_8(struct virtiox_softc *, int);
void virtiox_write_device_config_1(struct virtiox_softc *, int, uint8_t);
void virtiox_write_device_config_2(struct virtiox_softc *, int, uint16_t);
void virtiox_write_device_config_4(struct virtiox_softc *, int, uint32_t);
void virtiox_write_device_config_8(struct virtiox_softc *, int, uint64_t);

struct virtqueue * virtiox_alloc_vq(struct virtiox_softc *sc,
		int index, int size, const char *name);
void virtiox_free_vq(struct virtqueue*);
void virtiox_reset(struct virtiox_softc *);
void virtiox_reinit_start(struct virtiox_softc *);
void virtiox_reinit_end(struct virtiox_softc *);

struct vq_entry * vq_alloc_entry(struct virtqueue *vq);
void vq_free_entry(struct virtqueue *vq, struct vq_entry *qe);

int virtiox_enqueue_prep(struct virtiox_softc*, struct virtqueue*, int*);
int virtiox_enqueue_reserve(struct virtiox_softc*, struct virtqueue*, int, int);
//int virtiox_enqueue(struct virtiox_softc*, struct virtqueue*, int,
//		   bus_dmamap_t, bool);
//int virtiox_enqueue_p(struct virtiox_softc*, struct virtqueue*, int,
//		     bus_dmamap_t, bus_addr_t, bus_size_t, bool);
int virtiox_enqueue_commit(struct virtiox_softc*, struct virtqueue*, int, bool);
int virtiox_enqueue_abort(struct virtiox_softc*, struct virtqueue*, int);

int virtiox_dequeue(struct virtiox_softc*, struct virtqueue*, int *, int *);
int virtiox_dequeue_commit(struct virtiox_softc*, struct virtqueue*, int);

int virtiox_vq_intr(struct virtiox_softc *);
void virtiox_stop_vq_intr(struct virtqueue *);
void virtiox_start_vq_intr(struct virtqueue *);

void virtiox_show_features(struct virtiox_softc *sc, uint32_t features);
//void virtiox_ventry_stick(struct vq_entry *first, struct vq_entry *second);

void virtiox_ve_set(struct vq_entry *qe, ddi_dma_handle_t dmah,
	uint32_t paddr, uint16_t len, bool write);
void virtiox_push_chain(struct vq_entry *qe, boolean_t sync);
void virtiox_sync_vq(struct virtqueue *vq);

struct vq_entry * virtiox_pull_chain(struct virtqueue *vq, size_t *len);
void virtiox_free_chain(struct vq_entry *ve);

#endif /* _DEV_PCI_VIRTIOVAR_H_ */
