#include "jvm.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static volatile const char *g_jvm_stage = "idle";
const char *zev_jvm_current_stage(void){ return g_jvm_stage; }
static void stage(const char *s){ g_jvm_stage=s; fprintf(stderr,"JVM stage: %s\n",s); }

static int utf_eq(const uint8_t *base, uint16_t off, uint16_t len, const char *s){
    size_t n=strlen(s); return (size_t)len==n && memcmp(base+off,s,n)==0;
}
static uint16_t r16(const uint8_t **p){uint16_t v=((uint16_t)(*p)[0]<<8)|(*p)[1];*p+=2;return v;}
static uint32_t r32(const uint8_t **p){uint32_t v=((uint32_t)(*p)[0]<<24)|((uint32_t)(*p)[1]<<16)|((uint32_t)(*p)[2]<<8)|(*p)[3];*p+=4;return v;}
static int have(const uint8_t *p,const uint8_t *end,size_t n){return p<=end&&(size_t)(end-p)>=n;}

typedef struct {uint8_t tag;uint16_t a,b;uint32_t off;} CP;
typedef struct {uint16_t count;CP *cp;const uint8_t *method_data;const uint8_t *end;uint16_t methods;} Class;
typedef struct {const uint8_t *code;uint32_t len;} Code;

static int parse_class(const uint8_t*d,size_t n,Class*c){
    stage("parse class"); if(n<10||d[0]!=0xca||d[1]!=0xfe||d[2]!=0xba||d[3]!=0xbe)return-1;
    const uint8_t*p=d+8,*end=d+n;uint16_t count=r16(&p);c->count=count;c->end=end;
    c->cp=(CP*)calloc(count,sizeof(CP));if(!c->cp)return-2;
    for(uint16_t i=1;i<count;i++){
        if(!have(p,end,1)){free(c->cp);return-3;} uint8_t t=*p++;c->cp[i].tag=t;
        switch(t){
            case 1:{if(!have(p,end,2)){free(c->cp);return-3;}uint16_t l=r16(&p);c->cp[i].a=l;c->cp[i].off=(uint32_t)(p-d);if(!have(p,end,l)){free(c->cp);return-3;}p+=l;break;}
            case 3:case 4:if(!have(p,end,4)){free(c->cp);return-3;}c->cp[i].a=r16(&p);c->cp[i].b=r16(&p);break;
            case 5:case 6:if(!have(p,end,8)){free(c->cp);return-3;}r16(&p);r16(&p);r16(&p);r16(&p);if(++i<count)c->cp[i].tag=0;break;
            case 7:case 8:case 16:case 19:case 20:if(!have(p,end,2)){free(c->cp);return-3;}c->cp[i].a=r16(&p);break;
            case 9:case 10:case 11:case 12:case 17:case 18:if(!have(p,end,4)){free(c->cp);return-3;}c->cp[i].a=r16(&p);c->cp[i].b=r16(&p);break;
            case 15:if(!have(p,end,3)){free(c->cp);return-3;}r16(&p);c->cp[i].b=r16(&p);break;
            default:free(c->cp);return-5;
        }
    }
    if(!have(p,end,8)){free(c->cp);return-3;}r16(&p);r16(&p);r16(&p);r16(&p);
    if(!have(p,end,2)){free(c->cp);return-3;}uint16_t interfaces=r16(&p);if(!have(p,end,2u*interfaces)){free(c->cp);return-3;}p+=2u*interfaces;
    if(!have(p,end,2)){free(c->cp);return-3;}uint16_t fields=r16(&p);
    for(uint16_t i=0;i<fields;i++){if(!have(p,end,8)){free(c->cp);return-3;}r16(&p);r16(&p);r16(&p);uint16_t ac=r16(&p);for(uint16_t j=0;j<ac;j++){if(!have(p,end,6)){free(c->cp);return-3;}r16(&p);uint32_t z=r32(&p);if(!have(p,end,z)){free(c->cp);return-3;}p+=z;}}
    if(!have(p,end,2)){free(c->cp);return-3;}c->methods=r16(&p);c->method_data=p;return 0;
}

static int find_main(Class*c,const uint8_t*d,Code*out){
    stage("find main");const uint8_t*p=c->method_data;
    for(uint16_t i=0;i<c->methods;i++){
        if(!have(p,c->end,8)) return -3;
        (void)r16(&p);
        uint16_t ni=r16(&p);
        uint16_t di=r16(&p);
        uint16_t ac=r16(&p);
        Code code={0};
        for(uint16_t j=0;j<ac;j++){
            if(!have(p,c->end,6)) return -3;
            uint16_t ai=r16(&p);uint32_t z=r32(&p);
            if(!have(p,c->end,z)) return -3;
            const uint8_t*a=p;
            if(ai<c->count&&c->cp[ai].tag==1&&utf_eq(d,c->cp[ai].off,c->cp[ai].a,"Code")){
                if(z<12)return-4; r16(&a);r16(&a);uint32_t l=r32(&a);if(l>z-12)return-4; if(!have(a,c->end,l))return-3;code.code=a;code.len=l;
            }
            p+=z;
        }
        if(ni<c->count&&di<c->count&&c->cp[ni].tag==1&&c->cp[di].tag==1&&utf_eq(d,c->cp[ni].off,c->cp[ni].a,"main")&&utf_eq(d,c->cp[di].off,c->cp[di].a,"([Ljava/lang/String;)V")){*out=code;return code.code?0:-6;}
    }return-7;
}
static int cp_utf_eq(const Class*c,const uint8_t*d,uint16_t i,const char*s){return i<c->count&&c->cp[i].tag==1&&utf_eq(d,c->cp[i].off,c->cp[i].a,s);}
static int str_const(const Class*c,const uint8_t*d,uint16_t i,char*out,size_t cap){if(i==0||i>=c->count||c->cp[i].tag!=8)return-1;uint16_t u=c->cp[i].a;if(u>=c->count||c->cp[u].tag!=1)return-1;size_t n=c->cp[u].a;if(n>=cap)n=cap-1;memcpy(out,d+c->cp[u].off,n);out[n]=0;return 0;}
static int native_call(const Class*c,const uint8_t*d,uint16_t mr,int argc,int32_t*args){if(mr>=c->count||c->cp[mr].tag!=10)return-1;uint16_t ci=c->cp[mr].a,nt=c->cp[mr].b;if(ci>=c->count||nt>=c->count||c->cp[ci].tag!=7||c->cp[nt].tag!=12)return-1;uint16_t class_name=c->cp[ci].a,method_name=c->cp[nt].a;if(!cp_utf_eq(c,d,class_name,"ZevMobile"))return-1;if(cp_utf_eq(c,d,method_name,"boot"))return 0;if(cp_utf_eq(c,d,method_name,"println")&&argc==1){char s[256];if(str_const(c,d,(uint16_t)args[0],s,sizeof(s))==0)printf("%s\n",s);return 0;}if(cp_utf_eq(c,d,method_name,"setPixel")&&argc>=3)return 0;if(cp_utf_eq(c,d,method_name,"exit"))exit(argc?args[0]:0);return-1;}
static int invoke_static(const Class*c,const uint8_t*d,uint16_t mr,int32_t*stack,int*sp){if(mr>=c->count||c->cp[mr].tag!=10)return-1;uint16_t nt=c->cp[mr].b;if(nt>=c->count||c->cp[nt].tag!=12)return-1;uint16_t desc=c->cp[nt].b;if(desc>=c->count||c->cp[desc].tag!=1)return-1;if(cp_utf_eq(c,d,desc,"()V"))return native_call(c,d,mr,0,NULL);if(cp_utf_eq(c,d,desc,"(Ljava/lang/String;)V")){if(*sp<1)return-1;int32_t arg=stack[--*sp];return native_call(c,d,mr,1,&arg);}return-1;}

int jvm_run(const uint8_t*d,size_t n){
    Class c={0};Code code={0};int rc=parse_class(d,n,&c);if(rc){fprintf(stderr,"JVM: class parse failed %d\n",rc);return rc;}rc=find_main(&c,d,&code);if(rc){fprintf(stderr,"JVM: main not found (%d)\n",rc);free(c.cp);return rc;}
    stage("execute main");const uint8_t*p=code.code,*end=p+code.len;int32_t stack[256];int sp=0;
    while(p<end){uint8_t op=*p++;fprintf(stderr,"JVM opcode 0x%02X\n",op);if(op==0xb1)break;if(op==0xb8){if(!have(p,end,2)){free(c.cp);return-10;}uint16_t mr=r16(&p);stage("invokestatic");rc=invoke_static(&c,d,mr,stack,&sp);if(rc){free(c.cp);return rc;}continue;}if(op==0x12){if(!have(p,end,1)){free(c.cp);return-10;}uint8_t idx=*p++;if(idx<c.count&&c.cp[idx].tag==8){if(sp>=256){free(c.cp);return-11;}stack[sp++]=idx;continue;}free(c.cp);return-8;}if(op==0x10){if(!have(p,end,1)){free(c.cp);return-10;}int8_t v=(int8_t)*p++;if(sp>=256){free(c.cp);return-11;}stack[sp++]=v;continue;}if(op>=0x03&&op<=0x08){if(sp>=256){free(c.cp);return-11;}stack[sp++]=(int32_t)(op-0x03);continue;}if(op==0x57){if(sp)sp--;continue;}if(op==0xb7){if(!have(p,end,2)){free(c.cp);return-10;}uint16_t mr=r16(&p);if(mr<c.count&&c.cp[mr].tag==10){uint16_t ci=c.cp[mr].a,nt=c.cp[mr].b;if(ci<c.count&&nt<c.count&&c.cp[ci].tag==7&&c.cp[nt].tag==12){uint16_t cl=c.cp[ci].a,nm=c.cp[nt].a;if(cp_utf_eq(&c,d,cl,"java/lang/Object")&&cp_utf_eq(&c,d,nm,"<init>"))continue;}}free(c.cp);return-9;}free(c.cp);return-10;}
    stage("main returned");free(c.cp);return 0;
}

int zev_jvm_init(ZevJvm*jvm,ZevPhone*phone){if(!jvm)return-1;memset(jvm,0,sizeof(*jvm));jvm->phone=phone;stage("initialized");return 0;}
int zev_jvm_load_class(ZevJvm*jvm,ZevClassImage image){if(!jvm||!image.data||image.size==0){if(jvm)jvm->last_error=-1;return-1;}stage("load class");Class c={0};int rc=parse_class(image.data,image.size,&c);if(rc){jvm->last_error=rc;return rc;}free(c.cp);jvm->loaded_class=image;jvm->last_error=0;stage("class loaded");return 0;}
int zev_jvm_run_main(ZevJvm*jvm){if(!jvm||!jvm->loaded_class.data){if(jvm)jvm->last_error=-1;return-1;}jvm->running=1;int rc=jvm_run(jvm->loaded_class.data,jvm->loaded_class.size);jvm->running=0;jvm->exit_code=rc;jvm->last_error=rc;return 0==rc?0:rc;}
int zev_jvm_run_class(ZevJvm*jvm,ZevClassImage image){int rc=zev_jvm_load_class(jvm,image);if(rc)return rc;return zev_jvm_run_main(jvm);}
const char*zev_jvm_error_string(int e){switch(e){case 0:return"success";case-1:return"invalid JVM or class image";case-2:return"out of memory";case-3:return"truncated class file";case-4:return"invalid Code attribute";case-5:return"unsupported constant-pool tag";case-6:return"main method has no bytecode";case-7:return"main([Ljava/lang/String;)V not found";case-8:return"invalid ldc constant";case-9:return"unsupported invokespecial target";case-10:return"unsupported or malformed bytecode";case-11:return"operand stack overflow";default:return"unknown JVM error";}}
