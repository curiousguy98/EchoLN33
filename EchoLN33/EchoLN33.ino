#define ZIG_ADDRESS1 0xD801
#define ZIG_ADDRESS2 0xF801
unsigned int dest_address = ZIG_ADDRESS1;


#define countof(a) (sizeof(a) / sizeof(a[0]))
#define MAX_BUF_SIZE 64
char g_streamBuffer[MAX_BUF_SIZE]; 
char g_commandBuffer[MAX_BUF_SIZE];

char *g_cmd_current;
char *g_cmd_last;
bool g_cmd_dumped;                      // Indicates if last argument has been externally read 
bool g_cmd_ArgOk;

#define countof(a) (sizeof(a) / sizeof(a[0]))


unsigned char g_bufferIndex;
unsigned char g_bufferLength;
unsigned char g_bufferLastIndex; 

enum
{
  STATE_CMD_PROCESSING,
  STATE_CMD_DONE,
  STATE_CMD_ARGUMENT,
};

unsigned char g_CmdState;

void CmdReset()
{
  g_bufferIndex = 0;
  g_cmd_current = NULL;
  g_cmd_last = NULL;
  g_cmd_dumped = true;
}

int CmdFindNext(char *str, char delim)
{
  int pos = 0;

  bool EOL = false;

  while (true) {

    EOL = (*str == '\0');
    if (EOL) {
      return pos;
    }
    if (*str == ',') {
      return pos;
    }
    else {
      str++;
      pos++;
    }
  }
  return pos;
}

char* CmdSplit(char *str, const char delim, char **nextp)
{
  char *ret;
  // if input null, this is not the first call, use the nextp pointer instead
  if (str == NULL) {
    str = *nextp;
  }
  // Strip leading delimiters
  while (CmdFindNext(str, delim) == 0 && *str) {
    str++;
  }
  // If this is a \0 char, return null
  if (*str == '\0') {
    return NULL;
  }
  // Set start of return pointer to this position
  ret = str;
  // Find next delimiter
  str += CmdFindNext(str, delim);
  // and exchange this for a a \0 char. This will terminate the char
  if (*str) {
    *str++ = '\0';
  }
  // Set the next pointer to this char
  *nextp = str;
  // return current pointer
  return ret;
}

bool CmdNext()
{
  char * temppointer = NULL;
  switch (g_CmdState) {
  case STATE_CMD_PROCESSING:
    return false;
  case STATE_CMD_DONE:
    temppointer = g_commandBuffer;
    g_CmdState = STATE_CMD_ARGUMENT;
  default:
    if (g_cmd_dumped)
      g_cmd_current = CmdSplit(temppointer, ',', &g_cmd_last);
    if (g_cmd_current != NULL) {
      g_cmd_dumped = true;
      return true;
    }
  }


  return false;
}

char* ZigReadString()
{
  if (CmdNext()) {
    g_cmd_dumped = true;
    g_cmd_ArgOk = true;
    return g_cmd_current;
  }
  g_cmd_ArgOk = false;
  return 0;
}

unsigned int ZigReadInt16()
{
  if (CmdNext()) {
    g_cmd_dumped = true;
    g_cmd_ArgOk = true;
    return atoi(g_cmd_current);
  }
  g_cmd_ArgOk = false;
  return 0;
}

unsigned char ZigProcess(int rec_len)
{
  for (size_t byteNo = 0; byteNo < rec_len; byteNo++)
  {  
    g_CmdState = CmdProcessLine(g_streamBuffer[byteNo]);

    if (g_CmdState == STATE_CMD_DONE) {
      return 1;
    }
  }
  
  return 0;
}

unsigned char CmdProcessLine(char serialChar)
{
  g_CmdState = STATE_CMD_PROCESSING;

  if ((serialChar == ';')) {
    g_commandBuffer[g_bufferIndex] = 0;
    if (g_bufferIndex > 0) {
      g_CmdState = STATE_CMD_DONE;
      g_cmd_current = g_commandBuffer;

    }
    g_bufferIndex = 0;
    g_cmd_current = NULL;
    g_cmd_last = NULL;
    g_cmd_dumped = true;
  }
  else {
    g_commandBuffer[g_bufferIndex] = serialChar;
    g_bufferIndex++;
    if (g_bufferIndex >= g_bufferLastIndex)
      CmdReset();
  }
  return g_CmdState;
}

int ZigInit(unsigned int data_len, char * ZigData)
{

  g_bufferLength = MAX_BUF_SIZE;
  g_bufferLastIndex = MAX_BUF_SIZE - 1;
      
  unsigned char head = ZigData[0];
  if(head==0xFE){
    int data_len = ZigData[1] - 4;
    char port1 = ZigData[2];
    char port2 = ZigData[3];
    
    int src_address = ZigData[4]<<8 + ZigData[5];
    int index;
    for(index = 0;index < data_len; index++){
      g_streamBuffer[index] = ZigData[6+index];
    }
    
    g_streamBuffer[index] = 0;
    Serial.write(g_streamBuffer, index);
    Serial.println();
    
    return data_len;
  }

  return 0;
}

void setup() {
  // start serial port at 9600 bps
  Serial.begin(115200);
  Serial1.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
}

void loop() {

  bool recv_flag = false;
  while (Serial1.available())
  {
    recv_flag = true;

    char recvBuf[MAX_BUF_SIZE];
    size_t bytesAvailable = min(Serial1.available(), MAX_BUF_SIZE);
    Serial1.readBytes(recvBuf, bytesAvailable);

    int rec_len = ZigInit(bytesAvailable, recvBuf);

    if(ZigProcess(rec_len)){
      char* cmd = ZigReadString();
      Serial.println(cmd);
      int lastCommand = ZigReadInt16();
      Serial.println(lastCommand);
      lastCommand = ZigReadInt16();
      Serial.println(lastCommand);
      lastCommand = ZigReadInt16();
      Serial.println(lastCommand);
      lastCommand = ZigReadInt16();
      Serial.println(lastCommand);
      lastCommand = ZigReadInt16();
      Serial.println(lastCommand);

    }
  }

  if(!recv_flag){
    Serial.print("Sending to ");
    Serial.println(dest_address, HEX);
    char txBuffer[] = {0xFE, 0x0C, 0xA1, 0xA1,0xD8 ,0x01 ,
      0x62, 0x67, 0x2C, 0x32, 0x30, 0x31, 0x36, 0x2C, 0x38, 0x2C, 0x31, 0x36, 0x2C, 0x39, 0x2C, 0x33,
      0x31, 0x2C, 0x65, 0x64, 0x2C, 0x32, 0x30, 0x31, 0x36, 0x2C, 0x38, 0x2C, 0x31, 0x38, 0x2C, 0x32,
      0x32, 0x2C, 0x33, 0x31, 0x3B,
      0xFF};

    int cal_len = countof(txBuffer) - 3;
    txBuffer[1] = cal_len;
    txBuffer[4] = (dest_address & 0xFF00)>>8;
    txBuffer[5] = dest_address & 0x00FF;
    for (int i = 0; i < countof(txBuffer); i++){
      Serial1.write(txBuffer[i]);
    }
    
    delay(20000);
  }
  
}

