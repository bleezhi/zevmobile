#include "jvm.h"
#include <stdio.h>
#include <string.h>

#define STACK_MAX 256
#define LOCALS_MAX 64
#define CP_MAX 512

typedef struct { uint8_t tag; uint16_t a,b; int32_t off; } Cp;
typedef struct { const uint8_t *code; uint32_t len; } Code;
typedef struct { Cp cp[CP_MAX]; uint16_t count, methods; const uint8_t *method_data; } Class;

static uint16_t r16(const uint8_t **p){uint16_t v=((uint16_t)(*p)[0]<<8)|(*p)[1];*p+=2;return v;}
static uint32_t r32(const uint8_t **p){uint32_t v=((uint32_t)(*p)[0]<<24)|((uint32_t)(*p)[1]<<16)|((uint32_t)(*p)[2]<<8)|(*p)[3];*p+=4;return v;}
static const char *utf(Class *c,const uint8_t *base,uint16_t i){if(!i||i>=c->count||c->cp[i].tag!=1)return NULL;return (const char*)(base+c->cp[i].off);}
static int parse(Class *c,const uint8_t *d,size_t n){const uint8_t*p=d,*e=d+n;memset(c,0,sizeof(*c));if(n<10||r32(&p)!=0xCAFEBABE)return-1;r16(&p);r16(&p);c->count=r16(&p);if(c->count>=CP_MAX)return-1;for(uint16_t i=1;i<c->count;i++){uint8_t t=*p++;c->cp[i].tag=t;switch(t){case 1:{uint16_t z=r16(&p);c->cp[i].off=(int32_t)(p-d);c->cp[i].a=z;p+=z;break;}case 3:case 4:c->cp[i].off=(int32_t)r32(&p);break;case 5:case 6:p+=8;i++;break;case 7:case 8:case 16:c->cp[i].a=r16(&p);break;case 9:case 10:case 11:case 12:case 18:c->cp[i].a=r16(&p);c->cp[i].b=r16(&p);break;case 15:p++;r16(&p);break;default:return-1;}if(p>e)return-1;}r16(&p);r16(&p);r16(&p);uint16_t x=r16(&p);p+=x*2;uint16_t fields=r16(&p);for(uint16_t i=0;i<fields;i++){p+=6;uint16_t ac=r16(&p);for(uint16_t j=0;j<ac;j++){r16(&p);uint32_t z=r32(&p);p+=z;}}c->methods=r16(&p);c->method_data=p;return 0;}
static int find_main(Class*c,const uint8_t*d,Code*out){const uint8_t*p=c->method_data;for(uint16_t i=0;i<c->methods;i++){r16(&p);uint16_t ni=r16(&p),di=r16(&p),ac=r16(&p);Code code={0};for(uint16_t j=0;j<ac;j++){uint16_t ai=r16(&p);uint32_t z=r32(&p);const uint8_t*a=p;if(strcmp(utf(c,d,ai),"Code")==0){r16(&a);r16(&a);uint32_t l=r32(&a);code.code=a;code.len=l;}p+=z;}if(strcmp(utf(c,d,ni),"main")==0&&strcmp(utf(c,d,di),"([Ljava/lang/String;)V")==0){*out=code;return code.code?0:-1;}}return-1;}
static uint16_t ref_name(Class*c,const uint8_t*d,uint16_t ref){(void)d;uint16_t nt=c->cp[ref].b;return c->cp[nt].a;}
static uint16_t ref_desc(Class*c,uint16_t ref){uint16_t nt=c->cp[ref].b;return c->cp[nt].b;}
static const char*ref_class(Class*c,const uint8_t*d,uint16_t ref){uint16_t ci=c->cp[ref].a;return utf(c,d,c->cp[ci].a);}
static const char*str_const(Class*c,const uint8_t*d,uint16_t i){if(i>=c->count||c->cp[i].tag!=8)return NULL;return utf(c,d,c->cp[i].a);}
static int native(ZevJvm*j,Class*c,const uint8_t*d,uint16_t ref,intptr_t*a,int argc){const char*cl=ref_class(c,d,ref);const char*n=utf(c,d,ref_name(c,d,ref));if(!cl||!n||strcmp(cl,"ZevMobile"))return-1;if(!strcmp(n,"boot")){zev_phone_clear_screen(j->phone,0x00101018u);return 0;}if(!strcmp(n,"println")&&argc){printf("[JVM] %s\n",(const char*)a[0]);return 0;}if(!strcmp(n,"setPixel")&&argc>=3){zev_phone_set_pixel(j->phone,(int)a[0],(int)a[1],(uint32_t)a[2]);return 0;}if(!strcmp(n,"exit")){j->running=0;j->exit_code=argc?(int)a[0]:0;return 0;}return-1;}
static int run(ZevJvm*j,Class*c,const uint8_t*d,Code code){intptr_t s[STACK_MAX],l[LOCALS_MAX];int sp=0;memset(l,0,sizeof(l));const uint8_t*p=code.code,*e=p+code.len;while(p<e){uint8_t o=*p++;switch(o){case 0x00:break;case 0x01:s[sp++]=0;break;case 0x02 ... 0x08:s[sp++]=(intptr_t)o-3;break;case 0x10:s[sp++]=(int8_t)*p++;break;case 0x11:s[sp++]=(int16_t)r16(&p);break;case 0x12:s[sp++]=(intptr_t)str_const(c,d,*p++);break;case 0x13:s[sp++]=(intptr_t)str_const(c,d,r16(&p));break;case 0x15:case 0x19:s[sp++]=l[*p++];break;case 0x1a ... 0x1d:s[sp++]=l[o-0x1a];break;case 0x2a:s[sp++]=l[0];break;case 0x36:case 0x3a:l[*p++]=s[--sp];break;case 0x3b ... 0x3e:l[o-0x3b]=s[--sp];break;case 0x57:--sp;break;case 0x60:{intptr_t b=s[--sp],a=s[--sp];s[sp++]=a+b;break;}case 0x64:{intptr_t b=s[--sp],a=s[--sp];s[sp++]=a-b;break;}case 0x68:{intptr_t b=s[--sp],a=s[--sp];s[sp++]=a*b;break;}case 0x6c:{intptr_t b=s[--sp],a=s[--sp];if(!b)return-1;s[sp++]=a/b;break;}case 0xb1:return 0;case 0xb8:case 0xb6:case 0xb7:{uint16_t r=r16(&p);const char*q=utf(c,d,ref_desc(c,r));int argc=0;intptr_t a[8];const char*cl=ref_class(c,d,r);const char*name=utf(c,d,ref_name(c,d,r));if(o==0xb7&&cl&&name&&strcmp(cl,"java/lang/Object")==0&&!strcmp(name,"<init>")){break;}if(q){q=strchr(q,'(');if(q){for(++q;*q&&*q!=')';q++){if(*q=='L'){while(*q&&*q++!=';');}else if(*q=='['){while(*q=='[')q++;if(*q=='L')while(*q&&*q++!=';');}argc++;}}}if(argc>8||sp<argc)return-1;for(int k=argc-1;k>=0;k--)a[k]=s[--sp];if(native(j,c,d,r,a,argc)!=0)return-1;break;}default:return-1;}}return 0;}
int zev_jvm_init(ZevJvm*j,ZevPhone*p){memset(j,0,sizeof(*j));j->phone=p;return 0;}
int zev_jvm_load_class(ZevJvm*j,ZevClassImage im){Class c;(void)j;return parse(&c,im.data,im.size);}
int zev_jvm_run_main(ZevJvm*j){return j->running?0:0;}
int zev_jvm_run_class(ZevJvm*j,ZevClassImage im){Class c;Code code;if(parse(&c,im.data,im.size)||find_main(&c,im.data,&code))return-1;j->running=1;return run(j,&c,im.data,code);}
