ROOT=$(PWD)

ifeq ($(DESTDIR),)
DESTDIR=$(ROOT)
endif

world all:
	@cd virtio; gmake
	@cd virtio_blk; gmake

install:
	@cp virtio/virtio $(DESTDIR)/kernel/misc/amd64/
	@cp virtio_blk/vioblk $(DESTDIR)/kernel/drv/amd64/


manifest:
	cp manifest $(DESTDIR)/$(DESTNAME)

.PHONY: manifest

clean:
	@cd virtio; gmake clean
	@cd virtio_blk; gmake clean
