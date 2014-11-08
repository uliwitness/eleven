A small, modular, C++, BSD-socket-based chat client and server, intended for use in
projects that want to embed chat or chat-like functionality.


Warning
-------

This code is currently a work in progress and not suitable for use in production.
In particular, it is missing thread locking and there is no encryption, everything
goes across the wire in plain text.


Usage
-----

You can just telnet into the example server. So if you build and run it, it will
print the port it is listening on to the console. Just do

	telnet localhost 1234

(or whatever the port number was). Now you can type text line-wise into the Telnet
session and the server will process each line.

The example server is reasonably full-featured and implements a protocol vaguely
reminiscent of IRC. There is a "/login admin eleven" command to log you in
(the example is the default user name and password in a fresh build). A "/bye" command
to end your session. A "/join myChannel" command for creating and listening to a
channel. A "/leave myChannel" command to stop listening to a channel, commands to
create new users, ban users from a channel or even from just logging in, etc.

See the comments on each line that registers a handler in the example server's
main.cpp file for all commands currently supported.


Command handlers
----------------

The chat server class is stupid by default and does nothing. To make it do something, you
give it C++ lambdas (aka "blocks" or "handlers"). Each line received has its first word
extracted (everything up to the first space, tab or line break), and matched against the
strings for which handlers have been registered. If none of these matches, it looks for a handler
registered for "*".

These handlers get handed a session object with which they can send a reply, and
the entire line (including the first word) that the client sent. They can now
decide how to react.


Types of users
--------------

There are three kinds of users. "Normal" users can simply join/create channels and chat.
"Moderator" users can kick users from channels, block and retire users from logging in,
or delete a user, as long as the other user is a "Normal" user. "Owner" users can create
new accounts on the server, and designate other accounts as moderators or owners. Owners
can also do all the things moderators can.

When you build the example server, the Xcode project will copy a default accounts.txt
file next to the application that defines one user named "admin" with password "eleven".
This user is an "Owner" and can be used to create other users etc.


Format of messages
------------------

While messages sent from server to client can really have any format you wish (they can
even consist of binary data), the defaults have a strict format that will make it easier
to parse for clients and display them in a graphical fashion or localize them. All replies
from the server start with a four-character code. If this code starts with an exclamation
mark, it is an error (e.g. a message may start with "!AUT:" for "you are not logged in").

Then, messages usually contain one or more words (separated by spaces) indicating additional
metadata like the channel on which a message was sent, or the user name of the user that
originally sent the message. The rest of the line is usually the actual message sent, if any.

So if you were using Eleven to implement, say, the communications mechanism in an MMORPG,
the game the user downloads would be a client, and could choose to display all messages that
start with an exclamation mark in modal error alert panel. It could display messages that
start with "NOTE:" as text subtitles at the top of the game screen, and messages that start
with "JOIN:" could be meant to be the area in which the user is now playing. Messages that
start with "MESG:" (Actual text a user types) can even be shown as speech balloons over the
corresponding character's head, as they include channel and user name.

The server can even ask on behalf of the player to have the player join or leave a chatroom,
e.g. when the player moves from one map to the next, so that chat will show up in the current
room for all other players who are there with her. 


License
-------

    Copyright 2014 by Uli Kusterer.

    This software is provided 'as-is', without any express or implied
    warranty. In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
