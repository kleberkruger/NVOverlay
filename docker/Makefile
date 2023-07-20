# Dockerfile created by Kleber Kruger for using in LSC-Unicamp
# Recomendation: Others use Makefile.old

UBUNTU_VERSION?=20.04
DOCKER_IMAGE?=ubuntu:$(UBUNTU_VERSION)-sniper-$(USER)
DOCKER_FILE?=Dockerfile
# For use with --no-cache, etc.
DOCKER_BUILD_OPT?=
# Reconstruct the timezone for tzdata
TZFULL=$(subst /, ,$(shell readlink /etc/localtime))
TZ=$(word $(shell expr $(words $(TZFULL)) - 1 ),$(TZFULL))/$(word $(words $(TZFULL)),$(TZFULL))


all: $(DOCKER_FILE).build

# Use a .PHONY target to build all of the docker images if requested
Dockerfile.build: Dockerfile
	docker build --build-arg TZ_ARG=$(TZ) --build-arg GRAPHITE_ROOT_ARG=$(GRAPHITE_ROOT) --build-arg BENCHMARKS_ROOT_ARG=$(BENCHMARKS_ROOT) $(DOCKER_BUILD_OPT) -f $(<) -t $(DOCKER_IMAGE) .

run-root:
	docker run --rm -it -v "${HOME}:${HOME}" $(DOCKER_IMAGE)

run:
	docker run --rm -it -v "${HOME}:${HOME}" --user $(shell id -u):$(shell id -g) -w "${PWD}" $(DOCKER_IMAGE)

.PHONY: all run-root run
