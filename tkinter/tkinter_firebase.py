import firebase_admin
from firebase_admin import credentials
from firebase_admin import auth
from firebase_admin import db
from PIL import Image, ImageTk

import tkinter as tk

cred=credentials.Certificate('arduino-a344e-firebase-adminsdk-u2gib-f667149fc5.json')

firebase_admin.initialize_app(cred,
    {
        'databaseURL':'YOUR_DATABASE_URL'

    }
)

ref = db.reference('/test')
ref_status = db.reference('test/led state')

#create a window
window = tk.Tk()
window.title("LED switch")

def button1_click():
    
    status = ref_status.get()  
    ref_status.set(not status)
    button1.config(text="led lights:" + str(not status))

def update():
    status = ref_status.get()  
    button1.config(text="led lights:" + str(status))
    window.after(1000, update)

button1 = tk.Button(window, text="Click me!", height=10, width=10, command=button1_click)
button1.pack(side=tk.LEFT)

update()
window.mainloop()

