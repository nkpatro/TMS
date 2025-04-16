# TMS



The expected workflow is as follows:

- As this service starts loads the default config
- then it will check for the server status
- Wait for the Login, logout, lock, unlock events
- If the user alredy logged in when the service started, get the userId and auth token
- check if the machineId already exists (which is stored at the service first run)  else register machine and store the machineId
- check if there are any open sessions for the day, if not sent the retrieved userId and machineId to create a new session. we dont want to store the application info in the sessiondata
- watch for mouse and keyboard events on desktop or on application.
- if the application is not found on ther server, register them.
- track app_usage
- capture system matics (for future use.). This option can wait for future implementation.
