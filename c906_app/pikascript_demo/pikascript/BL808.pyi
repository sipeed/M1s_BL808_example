import PikaStdDevice


class GPIO(PikaStdDevice.GPIO):
    def platformHigh(self): ...

    def platformLow(self): ...

    def platformEnable(self): ...

    def platformDisable(self): ...

    def platformSetMode(self): ...

    def platformRead(self): ...

class Time(PikaStdDevice.Time):
    def sleep_s(self, s: int): ...

    def sleep_ms(self, ms: int): ...
