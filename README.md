# Toggled
<br>
<div align="center">
  <img src="https://github.com/Slate5/toggled/blob/main/etc/toggled.gif">
</div>

\
\
A simple tool to toggle *systemd* services, hence *toggled*.

*XFCE* doesn't have dynamic panel launchers, which poses a significant challenge for forgetful users. The user cannot see any change after clicking on a launcher. Since there is no dynamic icon change, how can the forgetful user know if they clicked on a launcher or were about to? Fear no more, forgetful users—`toggled` is here to help. It nonchalantly runs with root privileges so desktop entries can toggle services on/off without tapping into dark magic. Indeed, `toggled` is a bad boy.

## Compile/Install/Remove
```bash
$ [sudo] make [compile|install|remove] [y] [SERVICE={all|service_name}]
```
The position of the arguments is not important, more details are in the [*Makefile*](Makefile).

### Example
Install all desktop entries from the [*etc/*](etc/) without prompting:
```bash
$ sudo make install y SERVICE=all
```
Check *README* in [*etc/*](etc/) for more details about adding new desktop entries.

## Usage
```bash
$ toggled service [on|off]
```

### Example
`toggled` can be used as a *fidget toy*, allowing manic users to unleash their inner beast in two ways:
- Do more than 10 CPS on the *XFCE* panel launcher.
- Run `toggle minidlna` like hell in the terminal.

Or, to toggle the crap out of `minidlna`:
```bash
for (( i = 0; i < SRANDOM; ++i )); do
    toggled minidlna
done
```
Some services like to be `toggled`, others not so much (*start-limit-hit*). Toggle consensually...
