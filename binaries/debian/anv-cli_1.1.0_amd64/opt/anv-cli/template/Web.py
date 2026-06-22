from web import WebApp

app = None

def sayHello(val):
	print(f"Message from JS: {val}")

app = WebApp("index.html")
app.run()
