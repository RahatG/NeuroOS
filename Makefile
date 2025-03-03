# Simple wrapper around the script
.PHONY: all iso clean

all:
	@bash scripts/build_iso.sh

iso: all

clean:
	rm -rf build NeuroOS.iso
