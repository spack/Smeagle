.PHONY: all test standalone docs

all:
	cmake --log-level=VERBOSE -S all -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build

test:
	cmake -S test -B build/test
	cmake --build build/test
	CTEST_OUTPUT_ON_FAILURE=1 cmake --build build/test --target test

fmt:
	cmake -S test -B build/test
	cmake --build build/test --target format
	cmake --build build/test --target fix-format

standalone:
	cmake -S standalone -B build/standalone
	cmake --build build/standalone

docs:
	cmake -S documentation -B build/doc
	cmake --build build/doc --target GenerateDocs
	# open build/doc/doxygen/html/index.html
