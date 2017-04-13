// Pin config
#define D0 (2)
#define D7 (9)

#define PA0 (10)
#define PA1 (11)
#define WR (12)
#define RD (13)
#define RESET (14)

volatile String command = "";
volatile boolean commandAvailable = false;

// Set data bus I/O mode
void dataBusMode(uint8_t mode)
{
  int i;

  for (i = D0; i <= D7; i++) {
    pinMode(i, mode);
  }
}

// Init SHVC-SOUND
void initAPU(void)
{
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, LOW);

  dataBusMode(OUTPUT);

  pinMode(PA0, OUTPUT);
  pinMode(PA1, OUTPUT);

  pinMode(WR, OUTPUT);
  digitalWrite(WR, HIGH);

  pinMode(RD, OUTPUT);
  digitalWrite(RD, HIGH);

  delay(10);

  digitalWrite(RESET, HIGH);
}

void setup(void)
{
  Serial.begin(115200);

  initAPU();
}

void loop(void)
{
  if (!commandAvailable) {
    return;
  }

  switch (command[0]) {
    case 'I': // Init SHVC-SOUND
      initAPU();
      break;
    case 'R': // Read from SHVC-SOUND
      {
        if (command.length() != 2) {
          break;
        }

        uint8_t reg = command[1] - '0';

        dataBusMode(INPUT);

        // Set address ($2140 - $2143)
        digitalWrite(PA0, (reg & 1) ? HIGH : LOW);
        digitalWrite(PA1, (reg & 2) ? HIGH : LOW);

        // Set RD
        digitalWrite(RD, LOW);

        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");

        // Read data bus
        int i;
        uint8_t result = 0;

        for (i = D7; i >= D0; i--) {
          result <<= 1;
          result |= digitalRead(i);
        }

        // Print read data
        char output[4];
        sprintf(output, "R%02X", result);
        Serial.println(output);

        // Clear RD
        digitalWrite(RD, HIGH);

        break;
      }
    case 'W': // Write to SHVC-SOUND
      {
        if (command.length() != 4) {
          break;
        }

        uint8_t reg = command[1] - '0';

        dataBusMode(OUTPUT);

        // Set address ($2140 - $2143)
        digitalWrite(PA0, (reg & 1) ? HIGH : LOW);
        digitalWrite(PA1, (reg & 2) ? HIGH : LOW);

        uint8_t data = (uint8_t)strtol(command.substring(2).c_str(), NULL, 16);

        // Write data bus
        int i;
        uint8_t tmp = data;

        for (i = D0; i <= D7; i++) {
          digitalWrite(i, (tmp & 1) ? HIGH : LOW);
          tmp >>= 1;
        }

        // Set WR
        digitalWrite(WR, LOW);

        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");

        // Clear WR
        digitalWrite(WR, HIGH);

        break;
      }
  }

  command = "";
  commandAvailable = false;
}

// Serial I/O processing
void serialEvent(void)
{
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\n') {
      commandAvailable = true;
    } else {
      command += c;
    }
  }
}

