# Common deps
DEPS=Makefile

default: test_affinity test_numa_syscall test_numa_libnuma

test_affinity: $(DEPS) test-affinity.cpp
	g++ -O3 -g -o test_affinity test-affinity.cpp -pthread

test_numa_syscall: $(DEPS) test-numa-syscall.cpp
	g++ -O3 -g -o test_numa_syscall test-numa-syscall.cpp -lnuma

test_numa_libnuma: $(DEPS) test-numa-libnuma.cpp
	g++ -O3 -g -o test_numa_libnuma test-numa-libnuma.cpp --std=c++11 -pthread -lnuma

test_cc_exts: test-cc-exts.cpp $(DEPS)
	g++ -O3 -g -o $@ $< --std=c++11 -pthread

run_tests: default
	./test_affinity
	./test_numa_syscall
	./test_numa_libnuma
default: test_cc_exts


clean:
	rm -f *.o *.so *.a test_*

.PHONY: clean default

