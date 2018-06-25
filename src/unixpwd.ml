
module Stub = struct
    external get    :   string -> string              = "caml_getpwd"
    external setpwd :   string -> string -> unit      = "caml_setpwd"
    external setspw :   string -> string -> unit      = "caml_setspw"
    external unshadow : unit   -> string              = "caml_unshadow"
end
