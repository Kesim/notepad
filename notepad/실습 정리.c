10/15 월
1.환경변수 HBENV를 새로 정의하고 값을 hbooks로 설정
putenv("HBENV=hbooks");
val = getenv("HBENV");
printf("2. HBENV = %s\n", val);
========================
2.명령행 인자로 환경 변수명과 값을 입력받아 환경 변수를 설정하는 프로그램 작성
int main(int argc, char **argv) {
	setenv(argv[1], argv[2], 0);
============================
3.다른 match 함수를 사용하여 현재 디렉에서 명령어 인수로 끝나는 모든 파일 출력
main(int argc, char* argv[]) {
	DIR *dp;
	struct dirent *dent;
	struct stat sbuf;
	char path[BUFSIZ];
	
	if( (dp = opendir(argv[1])) == NULL) exit();// 3단계 에러
	
	while(dent = readdir(dp)) {
		if(dent->d_name[0] == '.') continue;
		sprintf(path, "%s/%s", argv[1], dent->d_name);
		stat(path, &sbuf);
		
		if(match(dent->d_name, argv[2])) printf("yes it is\n");
		
		closedir(dp);
	}
int match(const char *s1, const char *s2) {
	 int diff = strlen(s1) - strlen(s2);
	 if(diff > 0) return (strcmp(s1+diff, s2) == 0);
	 else return 0;
============================
4.현대 디렉 파일들을 ls -al 수행했을 때와 같이 출력
while(dent = readdir(dp)) {
	 if(dent->d_name[0] == '.') continue;
	 sprintf(path, "%s/%s", argv[1], dent->d_name);
	 stat(path, &sbuf);
	 
	 kind = sbuf.st_mode & S_IFMT;
	 switch(kind) {
		  case S_IFDIR : printf("d")
		  case S_IFREG : printf("-")
		  case S_IFLNK : printf("l")
	 }
	 
	 if( (sbuf.st_mode & S_IRUSR) != 0) printf("r"); else printf("-");
	 printf("%d ", sbuf.st_size);
	 printf("%.12s ", ctime(&(sbuf.st_mtime)));
	 
	 closedir(dp);
===============================
10/17 수
1.부모는 부모임을 자식은 자식임을 출력하는 프로그램
printf("I am parent.%d  my parent  has pid %d my child has %d\n"
                                , (int)getpid(), (int)getppid(), (int)pid);
===============================
2.두개의 부 프로세스 생성 후 각각의 자식 프로세스들은 그 자신의 자식 프로세스를 하나씩 생성하는 프로그램
case 0 : /* child process */
                        pid = fork();
                        pid = fork(); // 연속으로 두 번 포크
                        printf("Child Process - My PID: %d, My Parent's PID:%d\n",
                                (int)getpid(), (int)getppid());
================================
3.부모는 대문자 영문 출력
for(c = 'A'; c <= 'Z'; c++)  printf("%c ",c);
=================================
4.부모프로세스에서 개방된 파일은 자식 프로세스에서도 개방되고 공유됨.
=================================
10/29 월
1.fork로 생성된 자식에서 execl 과 exevp 써서 who 명령 실행
pos = lseek(fd, 10, SEEK_CUR); // 자식쪽에서 10바이트를 읽음
                        argv[0] = "who";
                        argv[1] = NULL;
                        if (execvp("who", argv) == -1) { 
2.현재 디렉에 있는 파일을 ls -al 형태로 출력하는		
             if (execlp("ls", "ls", "-al", (char*)NULL) == -1) {        
====================================
3.4개 자식 생성 후 1024개의 정수로 이루어진 파일을 동등하게 나눈 후, exec 함수를 사용해 자신에게 
할당된 부분을 읽고 값을 화면에 출력하는 프로그램을 작성하라
strcpy(cmdbuf, "od -i ");
strcat(cmdbuf, num);
system(cmdbuf);
cal.out--------------
#ifdef TIMES
	long sTime, eTime;
	struct timeval mytime;
	
	gettimeofday(&mytime, NULL);
	sTime = mytime.tv_usec;
#endif

lseek(fd, size/4*(num-1)*sizeof(int), SEEK_SET); /* 자기 읽는 부분 이동 */
read(fd, buf, sizeof(int)*size/num); /* 원하는 수를 버퍼에 읽고 */
for(i=0; i<size/4; i++) write(fd2, &buf[i], sizeof(int)); /* 한 자씩 파일에 쓰기 */
==================================
10/31 수
1.popen 사용해 부모가 보낸 내용이 자식에게 페이지 단위로
fp = popen("more", "w"); // 쪽 단위로 출력 //자식프로세스를 파일 읽듯이 읽기
 while(fgets(buf,256,fp2) != NULL) fputs(buf,fp); // 그 파일의 내용을 자식프로세스로 
 ====================
2.파이프 사용할때는 -------------
  while((n=read(fd[0], buf, 256)) <= 0); /* 부모로부터 내용을 읽음 */
 buf[n] = '\0'; /* 문자 열 끝 처리 */
 execlp("more", "more", buf, (char *)NULL); /* more 명령어의 인자로 넣어 줌 */ 
=====================
3.pipe 구현해서 wc -l 하기 
while((n = read(fd[0], buf, sizeof(buf))) <= 0); //end 표시가 올때까지 기다림
system("wc -l < 3-2data"); // 파일의 word count를 구함
혹은--------------
<자식>
dup2(fd[0], 0); /* 파이프 읽기를 표준 입력으로 */
execlp("wc", "wc", "-l", (char *)NULL); /* 이후 입력된 문장의 수를 셈 */
<부모>
dup2(fd[1], 1); /*dup2를 이용해 표준 출력을 파이프 쓰기로 */
for(a=1; a<=100; a++)
    write(1, "test line\n", 11);
==========================\
11/5 월
1.누군가 child가 각각 자신의 pipe를 가지고 부모에게 msg를 보냄, 부모는 select문을 사용하여
<메인>
for(i=0; i<SIZE; i++) {
	if(pipe(pip[i]) == -1) {
		switch(fork())
		case 0 : child(pip[i]);
parent(pip);
<부모>
FD_ZERO(&master);
FD_SET(0,&master);
        for(i=0; i<SIZE; i++) {
                FD_SET(p[i][0], &master);
        }
while(set = master, select(p[SIZE-1][0]+1, &set, NULL, NULL, NULL) > 0) {
for(i=0; i<SIZE; i++) {
       if(FD_ISSET(p[i][0], &set)) {
               nread = read(p[i][0], buf, MSGSIZE);
               write(1, buf, nread);
if(waitpid(-1, NULL, WNOHANG) == -1) return;
<자식>
write(p[1], "aaa", MSGSIZE);
==============================
2.양방향 파이프 ls -al | wc -l 구현
<자식>
if (fd[0] != 0) {
    dup2(fd[0], 0); // 부모로부터의 입력을 표준입력으로 바꿔놓고
    close(fd[0]);
}
execlp("wc", "wc", "-l", (char *)NULL); //exec씀 wc -l 가동
<부모>
if (fd[1] != 1) {
    dup2(fd[1], 1); // 자식으로가는 출력을 표준출력으로 바꾸고
    close(fd[1]);
}
execlp("ls", "ls", "-al", (char *)NULL); // exec로 ls -al 가동
==============================
3.자식-부모간 양방향 통신으로 hello world 주고받기
<자식>
write(fd2[1], hello, sizeof(hello));
len = read(fd1[0], buf, 256))
<부모>
len = read(fd2[0], hello, sizeof(hello)) 
write(fd1[1], ack, sizeof(ack));
===============================
11/7 수
1.클라가 표준입력한 내용을 서버가 표준출력으로
if (mkfifo("./HAN-FIFO", 0666) == -1) { // FIFO를 만들고
if ((pd = open("./HAN-FIFO", O_RDWR)) == -1) { //FIFO를 R/W 상태로 열기
2.FIFO 사용해서 1:1 대화가 가능한 양방향 프로그램 작성하라
<서버>
n = read(pd2[i], msg, sizeof(msg));
write(pd[i], ack, sizeof(ack));
<클라>
write(pd2[i], inmsg, strlen(inmsg));
read(pd[i], ackrcv, sizeof(ackrcv));
===============================
3.데드락 테스트 
first_lock.l_type = F_WRLCK;
first_lock.l_whence = SEEK_SET;
first_lock.l_start = 0;
first_lock.l_len = 10;

second_lock.l_type = F_WRLCK;
second_lock.l_whence = SEEK_SET;
second_lock.l_start = 10;
second_lock.l_len = 5;

if (fcntl(fd, F_SETLKW, &first_lock) == -1) {
	
case 0 :
		if (fcntl(fd, F_SETLKW, &second_lock) == -1) {

		if (fcntl(fd, F_SETLKW, &first_lock) == -1) { 
default:
		sleep(5);
		if (fcntl(fd, F_SETLKW, &second_lock) == -1) { 
==================================
11/14 수
1.클라가 보내는 메시지들을 server에서 mtype의 순번으로 받기
<초기화>
	long mtype;
	char mtext [MAXOBN+1];
key_t msq_key = ftok("keyfile", 1);
if ((queue_id = msgget(msq_key, IPC_CREAT | 0666)) == -1)
<클라>
msgbuf.mtype = (long)priority; // 우선순위 설정
if (msgsnd(id, &msgbuf, len, 0) == -1) { 
if ((mlen = msgrcv(r_qid, &r_entry, MAXOBN, (-1*MAXPRIOR), MSG_NOERROR)) == -1) 
r_entry.mtext[mlen] = '\0'; //널문자 삽입
=================================
2.자식이 표준 입력한 내용을 mtype=1로 부모에게 전달, 더이상 내용이 없을 때 mtype=2
<자식>
while(nread = read(0, buf, sizeof(buf)-1) > 0) {
	buf[nread] = 0;
	if(!strcmp(buf,"0")) break; /* 0은 종료 희망 */
	s_entry.mtype = 1; // 우선순위 설정
	if (msgsnd(s_qid, &s_entry, nread, 0) == -1) { // 메시지 보냄
<부모>
else if (mlen = msgrcv(r_qid, &r_entry, MAXOBN, 2, IPC_NOWAIT|MSG_NOERROR) > 0) {
	exit(0);
==================================
3. 1,3,3형 메시지 보내고 3번의 첫번째만 수신
<서버>
msgrcv(msgid, inmesg, 80, 3, 0); /* 80바이트이고 3형의 메시지를 블록방식으로 받음 */
=================================
4.client가 명령행 인자로 입력 받은 파일로부터 문자열을 연속적으로 보내고, server가 문자열을 화면으로 출력
if(mlen = msgrcv(r_qid, &r_entry, MAXOBN, 1, IPC_NOWAIT|MSG_NOERROR) > 0)  {
================================
11/19 월
1.unix와 programming이 같이 나오게 (이것이 섞이지 않게)
shmid = shmget(IPC_PRIVATE, 10*sizeof(int), IPC_CREAT|0644);
shmaddr = (int *)shmat(shmid, (int *)NULL, 0); /* 연결은 한번만 */

semid = semget(semkey, 1, IPC_CREAT | IPC_EXCL | 0666);
if(semid == -1) 
	 if(errno = EEXIST)
		 semid = semget(semkey, 1, 0);
else {
	semun.val = 1;
	int status = semctl(semid, 0, SETVAL, semun);
	
semlock(semid);
for (i=0; i<10; i++) {
	strcpy(shmaddr, "unix");
	printf("%s\n", shmaddr);
}
semunlock(semid);
shmdt(shmaddr);
shmctl(shmid, IPC_RMID, (struct shmid_ds *)NULL); /* 공유 메모리 제거 */


========================
10/17 수
1.system 의 리턴 값
a = system("ps -ef | grep UID > han.txt");
=========================
10/29 월
1.
if (execlp("ls", "ls", "-a", (char *)NULL) == -1) {
=======================
2
	argv[0] = "ls";
	argv[1] = "-a";
	argv[2] = NULL;
	if (execv("/bin/ls", argv) == -1) {
====================
10/31 수
1.해당 자식이 끝내지 않는이상 탈출 못함, 끝내고 나서 상태 코드 출력
while (wait(&status) != pid) continue;
printf("Child process Exit Status:%d\n", status >> 8);
=====================
2. 안끝났을 때 WNOHANG은 0을 리턴
while (waitpid(pid, &status, WNOHANG) == 0) {
		printf("Parent still wait..\n");
		sleep(1);
}
=====================
4. 간단한 파이프에 날짜 출력하고 그것을 부모가 읽기
fp = popen("date", "r");
if (fgets(buf, sizeof(buf), fp) == NULL) {
printf("line : %s\n", buf);
======================
5. 한 프로세스 내에서도 파이프 가능
	write(p[1], msg2, MSGSIZE);
	write(p[1], msg3, MSGSIZE);
	for (j=0; j<2; j++) {
		read(p[0], inbuf, MSGSIZE);
=========================
11/5 월
1.2초뒤에 함수를 호출 : 파이프에 얼마나 들어가는지 체크
#include <signal.h>
static struct sigaction act;
act.sa_handler = alrm_action;
sigfillset(&(act.sa_mask));
sigaction(SIGALRM, &act, NULL);
while (1) {
	alarm(2);
	write(p[1], &c, 1);
	if ((++count % 1024) == 0) printf("%d characters in pipe\n", count);
void alrm_action(int signo) {
======================
2.논블록 파이프
<메인>
if (fcntl(pfd[0], F_SETFL, O_NONBLOCK) == -1) 
<부모>
			switch (nread = read(p[0], buf, MSGSIZE)) {

			case -1:
				if (errno == EAGAIN) {
					printf("(pipe empty)\n");
					sleep(1);
					break;
				}
			case 0:
				printf("End of conversation\n");
				exit(0);
			default:
				printf("MSG = %s\n", buf);
		}
	}


=======================
3.select 연습, 자식이 다 끝나면 부모도 긑
fd_set set, master;
FD_ZERO(&master);
FD_SET(0, &master); /* 표준 입력 등록 */
for (i=0; i<3; i++) FD_SET(p[i][0], &master); /*읽기 파이프만 등록 */
while (set = master, select(p[2][0]+1, &set, NULL, NULL, NULL) > 0) {
	for (i=0; i<3; i++) {
				if (FD_ISSET(p[i][0], &set)) {
					if (read(p[i][0], buf, MSGSIZE) > 0) {
						printf("읽은 내용");
	if (waitpid (-1, NULL, WNOHANG) == -1) return; /* 모든 자식 종료 시 끝 */
========================
4. 파이프 이용 ps -ef | grep telnet 하기
            execlp("grep", "grep", "telnet", (char *)NULL);
            exit(1);
            break;
<부모>
        default :
            close(fd[0]);
            if (fd[1] != 1) {
                dup2(fd[1], 1);
                close(fd[1]);
            }
            execlp("ps", "ps", "-ef", (char *)NULL);
==========================
11/7 수
2.논블록 락 걸어서 어떤 프로세스가 락 걸었는지 파악하기
if (fcntl(fd, F_SETLK, &my_lock) == -1) {
				if (errno == EACCES || errno == EAGAIN) {
					fcntl(fd, F_GETLK, &b_lock);  /* 락을 받아오기 */
					printf("record locked by %d\n", b_lock.l_pid); /* 해당 PID */
========================
11/14 월
2.
printf(%s, ctime(&(mstat->msg_stime))); /* 마지막 전송 시간 */
msg_qnum /* 큐에 있는 메시지 수 */
========================
11/19 월
1.공유메모리로 상대에게 시그널 보내기
<클라>
kill(atoi(argv[1]), SIGUSR1);
<서버>
sigset_t mask;
	sigfillset(&mask);
	sigdelset(&mask, SIGUSR1);
	sigset(SIGUSR1, handler);
	sigsuspend(&mask);
===========================
3.세마포
typedef union semun {
	int val;
	struct semid_ds *buf;
	ushort *array;
}semun;

if (semid == -1) {
		if (errno == EEXIST)
			semid = semget(semkey, 1, 0);
}
else {
	semunarg.val = 1;
	status = semctl(semid, 0, SETVAL, semunarg);
}
-------------
buf.sem_num = 0;
buf.sem_op = -1;
buf.sem_flg = SEM_UNDO;
if (semop(semid, &buf, 1) == -1) {




