#
#
#

.PHONY: build release install uninstall clean test doc reindent

build:
	jbuilder build @install --dev

release:
	jbuilder build @install

install:
	jbuilder install

uninstall:
	jbuilder uninstall

clean:
	jbuilder clean

test:
	@echo "see test/ for test code. It requires root acceess."
	# jbuilder runtest

# requires odoc
doc:
	jbuilder build @doc

reindent:
	git ls-files '*.ml' '*.mli' | xargs ocp-indent --inplace
