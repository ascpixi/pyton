from something import abc

class range:
    def __init__(self, stop) -> None:
        self.start = 0
        self.stop = stop
        self.step = 1
        self.current = 0

    def __iter__(self):
        return self
    
    def __next__(self):
        if (self.step > 0 and self.current >= self.stop) or (self.step < 0 and self.current <= self.stop):
            raise StopIteration
        else:
            result = self.current
            self.current += self.step
            return result

for i in range(10):
    print("what is up")
    abc()
