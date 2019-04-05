# Watch-Folder
This program for MacOS (if you want to compile programs for other OS, you should change the directory path to Trash folder) will move files and folders to trash by schedule. Files and folders will move to Trash folder, files and folder in Trash will be deleted.

1. ## Edit crontab
Paste in terminal *crontab -e* and edit by following

#### Launch program every minute
\* * * * * /usr/bin/watchFolder
#### Launch program every 10 minutes
\*/10 * * * * /usr/bin/watchFolder

2. ## Edit watcFolder.conf (put it to /etc/watcFolder.conf)

* 1 s - second(s) old
* 1 m - minut(s) old
* 1 h - hour(s) old
* 1 d - day(s) old
The period may be from 1 up to 1000000000 (in seconds this is about 31,7 years :)

/Users/igrik/dir may be with spaces 6 h - (files in folder "/Users/igrik/dir may be with spaces" older than 6 hours will be moved to Trash folder)

**/Users/igrik/Downloads/ 1 h** - *(files in folder "/Users/igrik/Downloads/" older than 1 hours will be moved to Trash folder)*
**/Users/igrik/.Trash 1 d** - *(files in folder "/Users/igrik/.Trash " older than 1 day will be **removed**)*
