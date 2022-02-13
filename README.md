# Installation Instructions
1. Clone this git repo
2. Make sure `make` is installed, and run `make`
3. Run `make install`

# Uninstallation Instructions
1. Run `make uninstall` or just manually remove `tux_netspeed` binary
 from your path

# Integration with Tmux
1. Provided that `tux_netspeed` is installed, you can add the following
 to Tmux's configuration file:
	set-option -g status-right '#(tux_netspeed)'
