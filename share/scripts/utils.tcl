# several utility procs for usage in other scripts
# don't export anything, just use it from the namespace,
# because these scripts aren't useful for console users
# and should therefore not be exprted to the global
# namespace.
#
# these procs are not specific to anything special,
# they could be useful in any script.
#
# Born to prevent duplication between scripts for common stuff.

namespace eval utils {

proc get_machine_display_name { { machineid "" } } {
	if {$machineid == ""} {
		set machineid [machine]
	}
	if {$machineid == ""} {
		return "<none>"
	}
	set config_name [${machineid}::machine_info config_name]
	return [get_machine_display_name_by_config_name $config_name]
}

proc get_machine_display_name_by_config_name { config_name } {
	if {[catch {
		array set names [openmsx_info machines $config_name]
		set result [format "%s %s" $names(manufacturer) $names(code)]
	}]} {
		# hmm, XML file probably broken. Fallback:
		set result "$config_name (CORRUPT)"
	}
	return $result
}

proc get_machine_time { { machineid "" } } {
	if {$machineid eq ""} {
		set machineid [machine]
	}
	set err [catch {set mtime [${machineid}::machine_info time]}]
	if {$err} {
		return ""
	}
	return [format "%02d:%02d:%02d" [expr int($mtime / 3600)] [expr int($mtime / 60) % 60] [expr int($mtime) % 60]]
}

proc get_ordered_machine_list {} {
	return [lsort -dictionary [list_machines]]
}

} ;# namespace utils