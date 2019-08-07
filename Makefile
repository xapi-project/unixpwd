PROFILE=release
USER = unixpwd

.PHONY: build release install uninstall clean test doc reindent

build:
	dune build --profile=$(PROFILE) @install

release:
	dune build --profile=$(PROFILE) @install

install:
	dune install --profile=$(PROFILE)

uninstall:
	dune uninstall --profile=$(PROFILE)

clean:
	dune clean

test: build
	sudo useradd $(USER)
	sudo ./_build/default/test/main.exe $(USER)
	sudo userdel $(USER)

# requires odoc
doc:
	dune build @doc

reindent:
	git ls-files '*.ml' '*.mli' | xargs ocp-indent --inplace
