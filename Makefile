.PHONY: all build clean deconfig help install local remove sdk

build:
	cmake -H. -Bbuild -DBOARD_TYPE=
	cmake --build build --config Release

clean:
	rm -rf build

