// message.h
#ifndef MESSAGE_H
#define MESSAGE_H

// 메시지 타입 정의
typedef enum {
    MSG_CHAT = 1,       // 채팅 메시지
    MSG_FILE_START,     // 파일 전송 시작
    MSG_FILE_DATA,      // 파일 데이터 블록
    MSG_FILE_END        // 파일 전송 종료
} MessageType;

// 메시지 헤더 구조체
typedef struct {
    int type;   // 메시지 타입
    int size;   // 바디 길이
    int seq;    // 시퀀스 번호
} MessageHeader;

#endif // MESSAGE_H