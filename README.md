
# Unix Password

This is a simple C library with OCaml bindings to support updating Unix
account passwords in /etc/passwd and /etc/shadow. It is not meant to
support other operations, like creating or deleting accounts. For this
you might be interested in [opasswd] which exposed the complete C API
for password handling to [OCaml].

Be aware that updating password entries requires root privileges. So
update functions will fail unless called with root privileges.

[OCaml]:   https://www.ocaml.org/
[opasswd]: https://github.com/xapi-project/ocaml-opasswd.git

