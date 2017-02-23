# wee
Worst Editor Ever

# Concept
wee is a command line multi-user interactive text editor.

# Functionality
- Text editing (no GUI)
- Concurrent collaboration through server

# Schedule
|      |Joseph                           |Elena                                     |
|:-----|:--------------------------------|:-----------------------------------------|
|  3/1 |Launch Django Server             |Develop text editor interface             |
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



Joseph
1. Use Django with Python for the server, running on the school's virtual machine
2. Mechanism on sharing text file would be
	- Generate code when a user wants to share
	- When the text editor generates the common code, it would copy the text file and 
		upload it to the server
	- As soon as it uploads to the server, it should be available for any users with 
		the common code to access and edit
	- The key here is that all accessed users should be able to see the real-time editing
		and to edit in real-time 
	- diff would be used effectiely
