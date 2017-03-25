struct RCB{
  public :
  int sequenceNumber;
  int fileDescriptor;
  char* fileHandle;
  int bytesRemaining;
  int byteQuantum;

  void setSequenceNumber(int number){
    sequenceNumber = number;
  }
  int getSequenceNumber() {
    return sequenceNumber;
  }
  void setFileDecriptor(int fileDescriptor) {
    this.fileDesriptor = fileDescriptor;
  }
  int getFileDecriptor() {
    return fileDescriptor;
  }
  void setFileHandle()
};
