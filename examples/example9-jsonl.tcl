package require tjson 1.0.29

set path [file join [file dirname [info script]] events.jsonl]
set ch [open $path rb]
try {
    while {[::tjson::jsonl_gets $ch record]} {
        set event [::tjson::typed_to_simple $record]
        puts "User [dict get $event user]: [dict get $event event]"
    }
} finally {
    close $ch
}
