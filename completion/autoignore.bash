_autoignore() {
    local cur prev
    _init_completion || return

    case "$prev" in
        -o|--output)
            _filedir
            return
            ;;
        -s|--search)
            return
            ;;
    esac

    if [[ "$cur" == -* ]]; then
        COMPREPLY=($(compgen -W \
            '-l --list -s --search -i --interactive -d --detect
             -o --output -a --append -p --preview -v --verbose -h --help' \
            -- "$cur"))
        return
    fi

    local templates
    templates=$(autoignore --list 2>/dev/null | awk '/^  [a-z]/{print $1}')
    COMPREPLY=($(compgen -W "$templates" -- "$cur"))
}

complete -F _autoignore autoignore
