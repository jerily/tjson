set dir [file dirname [info script]]

package ifneeded tjson 0.1 [list load [file join $dir libtjson.so]]
