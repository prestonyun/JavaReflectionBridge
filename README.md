# Java Reflection Bridge (JRB) Library

The Java Reflection Bridge (JRB) is a dynamic linking library written in C++ designed to provide a communication bridge between a Java Virtual Machine (JVM) and external applications, initially aimed at Python applications but adaptable to other languages. The library, once compiled into a DLL file and injected into a Java process, establishes a named pipe, listening for incoming connections. On receiving a message, it interprets the message as a method call on a Java object, performs the necessary reflection to execute the method, and sends the result back to the external application.

## Features

- Establishes a named pipe for communication between JVM and external applications.
- Parses incoming messages as Java method calls (e.g., `client.getGameState()`).
- Utilizes Java reflection to discover and cache method signatures, IDs, return types, and reference objects from the JVM.
- Executes the method calls using the Java Native Interface (JNI) reflection.
- Returns the result of the method execution back to the external application.

## Getting Started

### Prerequisites

- A modern C++ compiler
- Java Development Kit (JDK)

### Compilation

Compile the C++ source files into a DLL using your preferred C++ compiler. The compiled DLL is ready to be injected into a Java process.

### Injector

To inject the compiled DLL into the target Java process, you can use the injector utility available in [this repository](https://github.com/prestonyun/Injector).

### Usage

1. Inject the compiled DLL into the target Java process using the injector utility.
2. Establish a connection to the named pipe created by the library.
3. Send Java method call messages through the pipe.
4. Receive and process the results sent back through the pipe.

#### Python Example

```python
class JWrapper:
    def __init__(self, targetClass):
        self.targetClass = targetClass
        self.method_chain = []
        self.api = RemoteAPI()

    def __getattr__(self, methodName):
        return ChainProxy(self, methodName)

    def execute(self):
        query = f"{self.targetClass}." + ".".join(self.method_chain)
        self.method_chain = []
        return self.api.query(query)

class ChainProxy:
    def __init__(self, parent: JWrapper, methodName: str):
        self.parent = parent
        self.methodName = methodName

    def __getattr__(self, nextMethodName):
        self.parent.method_chain.append(self.methodName)
        return ChainProxy(self.parent, nextMethodName)

    def __call__(self, *args):
        args_str = ', '.join(map(str, args))
        method_with_args = f"{self.methodName}({args_str})"
        self.parent.method_chain.append(method_with_args)
        return self.parent

class RemoteAPI:
    def __init__(self, encoding='utf-8'):
        try:
            pid = find_game_client_pid()
            injector.Injector.inject(os.path.join(os.path.dirname(os.path.abspath(__file__)), "ClientReflection.dll"), find_game_client_pid())
        except Exception as e:
            print("Error injecting JShell.dll: ", e)
        self.pipe_name = r'\\.\pipe\clientpipe'
        self.handle = None
        self.encoding = encoding
        self.lock = threading.Lock()  # For thread safety

    def write_to_pipe(self, message: str) -> bool:
        with self.lock:
            try:
                win32file.WriteFile(self.handle, message.encode(self.encoding))
                return True
            except Exception as e:
                print(f"Error writing to pipe: {e}")
                return False

    def read_from_pipe(self, buffer_size=32768) -> str:
        with self.lock:
            try:
                result, data = win32file.ReadFile(self.handle, buffer_size)
                return data.decode(self.encoding)
            except Exception as e:
                print(f"Error reading from pipe: {e}")
                return None

    def __enter__(self):
        with self.lock:
            try:
                self.handle = win32file.CreateFile(
                    self.pipe_name,
                    win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                    0,
                    None,
                    win32file.OPEN_EXISTING,
                    0,
                    None
                )
            except pywintypes.error as e:
                print(f"Could not open pipe: {e}")
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        with self.lock:
            if self.handle:
                try:
                    win32file.CloseHandle(self.handle)
                    self.handle = None
                except Exception as e:
                    print(f"Error closing pipe: {e}")

    def query(self, script: str):
        assert isinstance(script, str)
        with self:
            self.write_to_pipe(script)
            response = self.read_from_pipe()
        return response


if __name__ == '__main__':
    client = JWrapper("Client")
    print(client.getGameState().execute())

# Example output:
# LOGIN_SCREEN
```

## Adapting to Other Languages

Though initially designed for interfacing with Python, the JRB library can be adapted to support other languages. The primary requirement is the ability of the external application to communicate through a named pipe.

## Contribution

Contributions to the JRB library are welcome. Please fork the repository, make your changes, and submit a pull request.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.
