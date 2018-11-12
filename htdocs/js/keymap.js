var keymap = {
    "ArrowLeft": {"cmd": "clickPrev", "options": [], "action": "Previous Song", "key": "keyboard_arrow_left"},
    "ArrowRight": {"cmd": "clickNext", "options": [], "action": "Next Song", "key": "keyboard_arrow_right"},
    " ": {"cmd": "clickPlay", "options": [], "action": "Toggle Play / Pause", "key": "space_bar"},
    "s": {"cmd": "clickStop", "options": [], "action": "Stop Playing"},
    "-": {"cmd": "chVolume", "options": [-5], "action": "Volume Down"},
    "+": {"cmd": "chVolume", "options": [5], "action": "Volume Up"},
    "c": {"cmd": "MPD_API_QUEUE_CLEAR", "options": [], "action": "Clear Queue"},
    "u": {"cmd": "updateDB", "options": [], "action": "Update Database"},
    "r": {"cmd": "rescanDB", "options": [], "action": "Rescan Database"},
    "l": {"cmd": "openLocalPlayer", "options": [], "action": "Open Local Player"},
    "p": {"cmd": "updateSmartPlaylists", "options": [], "action": "Update Smart Playlists"},
    "a": {"cmd": "showAddToPlaylist", "options": ["stream"], "action": "Add Stream"},
    "t": {"cmd": "openModal", "options": ["modalSettings"], "action": "Open Settings"},
    "y": {"cmd": "openModal", "options": ["modalAbout"], "action": "Open About"},
    "i": {"cmd": "clickTitle", "options": [], "action": "Open Song Details"},
    "1": {"cmd": "appGoto", "options": ["Playback"], "action": "Goto Playback"},
    "2": {"cmd": "appGoto", "options": ["Queue"], "action": "Goto Queue"},
    "3": {"cmd": "appGoto", "options": ["Browse","Database"], "action": "Goto Browse Database"},
    "4": {"cmd": "appGoto", "options": ["Browse","Playlists"], "action": "Goto Browse Playlists"},
    "5": {"cmd": "appGoto", "options": ["Browse","Filesystem"], "action": "Goto Browse Filesystem"},
    "6": {"cmd": "appGoto", "options": ["Search"], "action": "Goto Search"},
    "m": {"cmd": "openDropdown", "options": ["dropdownMainMenu"], "action": "Open Main Menu"},
    "v": {"cmd": "openDropdown", "options": ["dropdownVolumeMenu"], "action": "Open Volume Menu"},
    "S": {"cmd": "MPD_API_QUEUE_SHUFFLE", "options": [], "action": "Shuffle Queue"},
    "C": {"cmd": "MPD_API_QUEUE_CROP", "options": [], "action": "Crop Queue"},
    "?": {"cmd": "openModal", "options": ["modalHelp"], "action": "Open Help"},
    "/": {"cmd": "focusSearch", "options": [], "action": "Focus Search"}
}
