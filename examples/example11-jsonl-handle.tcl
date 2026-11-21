package require tjson 1.0.29

set path [file join [file dirname [info script]] events.jsonl]
set ch [open $path rb]
try {
    while {[::tjson::jsonl_gets -handle $ch root]} {
        try {
            foreach match [::tjson::query $root {$.event}] {
                puts [::tjson::to_simple $match]
            }
        } finally {
            ::tjson::destroy $root
        }
    }
} finally {
    close $ch
}
