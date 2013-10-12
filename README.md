mini-shell
==========

minimal shell-like executable realised as coursework for UCL COMP3005 Module.


COMP2013 assignment in brief.

i) Upon startup the shell gets the current working dir, read the profile file in it and set two variables PATH and HOME accordingly. An error is reported if any of the two variables or the config file are missing.
ii) The shell shows a prompt structured like $HOME> i.e. /home/user>
iii) when the user types a command the first word is interpreted as the name of the command to be searched in $PATH, the other words are treated as parameters.
iv) when the program completes the prompt should be shown again
v) cd commands and PATH or HOME assignments in the shell should work