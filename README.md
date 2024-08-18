# Keyboard Layout cyler

This is a simple app to cycle between layouts in Linux using `setxkbmap`.

## Configuration

The configuration file is located at `~/.config/keyboardLayoutCycler/config.toml`.

Default configuration:
```toml
layout_list = [ "<your_current_layout>" ]
```
`layout_list` is the list of layouts to cycle through. They have to be a valid value for `setxkbmap`.


## Usage

You just need to run the app and send the `SIGRTMIN`(34) signal to it. You can use `kill -34 <pid>`

