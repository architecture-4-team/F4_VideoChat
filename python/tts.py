import pyttsx3

def text_to_speech(text):
    engine = pyttsx3.init()
    engine.setProperty('rate', 150)  # 속도 설정 (기본 값은 200)
    engine.say(text)
    engine.runAndWait()

# 텍스트를 음성으로 변환하고 재생합니다.
text_to_speech("Incomming Call from dy.test.com.")

# 원하는 텍스트를 전달하여 함수를 호출하여 음성으로 변환 및 재생할 수 있습니다.
