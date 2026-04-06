function __autoignore_templates
    autoignore --list 2>/dev/null | awk '/^  [a-z]/{print $1}'
end

complete -c autoignore -s l -l list        -d 'List available templates'
complete -c autoignore -s s -l search      -d 'Search templates by name' -r
complete -c autoignore -s i -l interactive -d 'Select templates interactively'
complete -c autoignore -s d -l detect      -d 'Auto-detect templates from project files'
complete -c autoignore -s o -l output      -d 'Output file' -r -F
complete -c autoignore -s a -l append      -d 'Append to existing file'
complete -c autoignore -s p -l preview     -d 'Preview output without writing'
complete -c autoignore -s v -l verbose     -d 'Verbose output'
complete -c autoignore -s h -l help        -d 'Show help'
complete -c autoignore -f -a '(__autoignore_templates)'
