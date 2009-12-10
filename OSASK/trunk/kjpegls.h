/* ハフマンテーブル */
typedef struct{
  int elem; /* 要素数 */
  unsigned short code[256];
  unsigned char  size[256];
  unsigned char  value[256];
} HUFF;

typedef struct{
  /* SOF */
  int width;
  int height;
  /* MCU */
  int mcu_width;
  int mcu_height;

  int max_h,max_v;
  int compo_count;
  int compo_id[3];
  int compo_sample[3];
  int compo_h[3];
  int compo_v[3];
  int compo_qt[3];

  /* SOS */
  int scan_count;
  int scan_id[3];
  int scan_ac[3];
  int scan_dc[3];
  int scan_h[3];  /* サンプリング要素数 */
  int scan_v[3];  /* サンプリング要素数 */
  int scan_qt[3]; /* 量子化テーブルインデクス */
    
  /* DRI */
  int interval;

  int mcu_buf[32*32*4]; /* バッファ */
  int *mcu_yuv[4];
  int mcu_preDC[3];
    
  /* DQT */
  int dqt[3][64];
  int n_dqt;
    
  /* DHT */
  HUFF huff[2][3];
  
    
  /* FILE i/o */
  unsigned char *fp, *fp1;
  /*    FILE *fp; */
  unsigned long bit_buff;
  int bit_remain;

  int width_buf;

} JPEG;

void jpeg_init(JPEG *jpeg);
int jpeg_header(JPEG *jpeg);
void jpeg_decode(JPEG *jpeg, unsigned char *rgb);
