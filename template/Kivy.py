import kivy
from kivy.app import App
from kivy.uix.label import Label

class SimpleLabelApp(App):
    def build(self):
        return Label(text='Hello Kivy')

SimpleLabelApp().run()
