#ifndef F2B_CMSG_H_
#define F2B_CMSG_H_

#include <sys/uio.h>

#define DATA_LEN_MAX 496 /* 512 - 16 bytes of header */
#define DATA_ARGS_MAX 6  /* number of args in data */
#define F2B_PROTO_VER 1

enum f2b_cmsg_type {
  CMD_NONE = 0,
  CMD_RESP,
  CMD_HELP,
  CMD_PING = 8,
  CMD_STATUS,
  CMD_ROTATE,
  CMD_RELOAD,
  CMD_SHUTDOWN,
  CMD_JAIL_STATUS = 16,
  CMD_JAIL_SET,
  CMD_JAIL_IP_SHOW,
  CMD_JAIL_IP_BAN,
  CMD_JAIL_IP_RELEASE,
  CMD_MAX_NUMBER,
};

#define CMSG_FLAG_NEED_REPLY 0x01
#define CMSG_FLAG_AUTH_PASS  0x02

/**
 * @struct f2b control message
 *
 * Use sendmsg/recvmsg and iovec structs to pack/unpack
 */
typedef struct f2b_cmsg_t {
  char magic[3];   /**< magic string "F2B" */
  uint8_t version; /**< protocol version */
  /* 4 bytes */
  uint8_t type;    /**< command type, cast from enum f2b_cmsg_type */
  uint8_t flags;   /**< CMSG_FLAG_* */
  uint16_t size;   /**< payload length */
  /* 8 bytes */
  char pass[8];
  /* 16 bytes */
  /* end of header */
  char data[DATA_LEN_MAX];      /**< set of "\n"-terminated strings */
  /* end of data */
} f2b_cmsg_t;

void f2b_cmsg_convert_args(f2b_cmsg_t *msg);
void f2b_cmsg_extract_args(const f2b_cmsg_t *msg, const char **argv);

#endif /* F2B_CMSG_H_ */
