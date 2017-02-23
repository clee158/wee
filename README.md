# wee
Worst Editor Ever

# Concept
wee is a command line multi-user interactive text editor.

# Functionality
- Text editing (no GUI)
- Concurrent collaboration through server

# Features
- Insert/Delete text anywhere in the file
- Scroll through the file using arrow keys
- Copy/cut text and paste anywhere in the file
- Command-based interface

# Schedule
|      |Joseph                           |Elena                                     |
|:-----|:--------------------------------|:-----------------------------------------|
|  3/1 |Launch Server				             |Develop text editor interface             |
|  3/8 |Networking with the server       |Develop text editor interface             |
|  3/15|Develop text editor interface    |Develop text editor interface             |
|  3/22|Develop text editor interface    |Single-user networking with the server    |
|  3/29|Develop text editor interface    |Multi-user networking with the same server|
|  4/5 |Real-time networking while on wee|Real-time networking while on wee         |
|  4/12|Concurrency                      |Concurrency                               |
|  4/19|Debugging                        |Debugging                                 |
|  4/26|Final wrap up                    |Final wrap up                             |
-------------------------------------------------------------------------------------

# Research results
Elena
1. Potentially use curses library with python/C
2. Need a data structure
	- possibly the Vector structure we implemented for CS241

Joseph
1. Use VM to run the server
2. Mechanism on sharing text file would be
	- Generate code when a user wants to share
	- When the text editor generates the common code, it would copy the text file and 
		upload it to the server
	- As soon as it uploads to the server, it should be available for any users with 
		the common code to access and edit
	- The key here is that all accessed users should be able to see the real-time editing
		and to edit in real-time 
	- diff would be used effectiely
