#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>
#include<windows.h>
#include<assert.h> 

#define speed 10 //speed:10(m/s)
#define sec_per_turn 75
#define handoff_per_turn 125
#define max_xy 3000
#define min_xy 0
#define crossing_num 21 
#define corner_num 4
#define entry_num 12 //entry point
#define bs_num 4
#define policy_num 4
#define Threshold -110
#define Entropy 5 
 
struct car
{
	int id;
	int x;
	int y;
	int direct; //up:1 down:3 right:0 left:2
	int time;
	char turn_flag; //Did the car turn last round
	double p[bs_num]; //power for four base station
	int current_bs[policy_num]; //0~3
	struct car* next;
};

const int crossing_x[crossing_num] = {750, 1500, 2250, 750, 1500, 2250, 750, 1500, 2250,
	750, 1500, 2250, 3000, 3000, 3000, 2250, 1500, 750, 0, 0, 0};
const int crossing_y[crossing_num] = {750, 750, 750, 1500, 1500, 1500, 2250, 2250, 2250,
 	0, 0, 0, 750, 1500, 2250, 3000, 3000, 3000, 2250, 1500, 750};
const int entry_x[entry_num] = {750, 1500, 2250, 3000, 3000, 3000, 2250, 1500, 750, 0, 0, 0};
const int entry_y[entry_num] = {0, 0, 0, 750, 1500, 2250, 3000, 3000, 3000, 2250, 1500, 750};
const int bs_x[bs_num] = {750, 2250, 2250, 750};
const int bs_y[bs_num] = {750, 750, 2250, 2250};
const int corner_x[corner_num] = {0, 3000, 3000, 0};
const int corner_y[corner_num] = {0, 0, 3000, 3000};

struct car * t_list = NULL;
int hang_off_num[policy_num] = {0, 0, 0, 0};
double total_power[policy_num] = {0.0, 0.0, 0.0, 0.0};
int car_cnt = 0;

void drive(struct car *, int);
int turn(int);
void car_in(int, int);
void compute_power();
void car_out();
int corner_test(int, int);
int search_by_pos(int, int);
void hand_off(struct car *);
int distance_i(int,int,int,int);
double distance_d(double,double,double,double);
int max_of_arr(double*, int); //return the max item's index 
void print();

int main()
{
	struct car * t = NULL;
	int i, j; 
	double avg_p;
	FILE *fp1, *fp2, *fp3, *fp4;
	srand(time(NULL));
	
	fp1 = fopen("file1.txt","w");
	assert(fp1!=NULL);
	fp2 = fopen("file2.txt","w");
	assert(fp2!=NULL);
	fp3 = fopen("file3.txt","w");
	assert(fp3!=NULL);
	fp4 = fopen("file4.txt","w");
	assert(fp4!=NULL);
	
	for(i=0 ; i<86400 ; ++i)
	{		
		t = t_list;	
		for(j=0 ; j<entry_num ; ++j)
		{
			car_in(entry_x[j], entry_y[j]);
		}
		while(t!=NULL)
		{
			if(t->time%sec_per_turn==0)
			{
				drive(t, 1);
			}
			else
			{
				drive(t, 0);
			}
			t->time++;
			t = t->next;
		}
		car_out();
		compute_power();
		fprintf(fp1, "%d\n", hang_off_num[0]);
		fprintf(fp2, "%d\n", hang_off_num[1]);
		fprintf(fp3, "%d\n", hang_off_num[2]);
		fprintf(fp4, "%d\n", hang_off_num[3]);
		/*printf("****%d*****\n",i);
		print();
		printf("*********\n\n");	
		Sleep(600);*/ 
	}
	printf("%d, %d, %d, %d\n",hang_off_num[0],hang_off_num[1],hang_off_num[2],hang_off_num[3]);
	for(i=0; i<policy_num ; ++i)
	{
		avg_p = total_power[i]/86400/car_cnt;
		printf("%lf, ",10*log10(avg_p));
	}
	printf("\n");
	fclose(fp1);
	fclose(fp2);
	fclose(fp3);
	fclose(fp4);
	return 0;
}

void car_in(int x, int y)
{	
	int a = rand()%1000, i;
	struct car * in = NULL;
	struct car * t = NULL;
	if(a<32) //p=0.032
	{
		in= malloc(sizeof(struct car));
		in->x = x;
		in->y = y;
		in->turn_flag = 0;
		in->time = 0;
		if(in->x == min_xy)
		{
			in->direct = 0;
		}
		else if(in->y == min_xy)
		{
			in->direct = 3;
		}
		else if(in->x == max_xy)
		{
			in->direct = 2;
		}
		else if(in->y == max_xy)
		{
			in->direct = 1;
		}
		in->next = NULL;
		in->id = ++car_cnt;
		
		//assign Base station to each car
		//initialization with choosing the most powerful Base station
		double d;
		for(i=0 ; i<bs_num ; ++i)
		{
			d = distance_d((double)in->x, (double)in->y, (double)bs_x[i], (double)bs_y[i]);
			if(d <= 1.0)
				in->p[i] = -60;
			else
				in->p[i] = -60 - 20*log10(d);
		}
		for(i=0 ; i<policy_num ; ++i)
			in->current_bs[i] = max_of_arr(in->p, bs_num);
		
		//connect to the linked list
		if(t_list==NULL)
			t_list = in;
		else
		{
			t= t_list;
			while(t->next!=NULL)
				t = t->next;
			t->next = in;
		}	
	}
	return ;
}

int turn(int t)
{
	int a = rand()%100;
	if(a<50) //0.5 straight
		return t;
	else if(a>=50&&a<=82) //0.33 right
	{
		if(t-1<0)
			return 3;
		else
			return t-1;
	}
	else if(a>=82) //0.17 left
	{
		if(t+1>3)
			return 0;
		else
			return t+1;
	}	
}

void drive(struct car *c, int flag)
{
	int pre_x = c->x, pre_y = c->y;
	int post_x, post_y;
	int d, i;
	if(flag==1)
	{
		for(i=0 ; i<corner_num ; ++i)
		{
			if(pre_x == corner_x[i] && pre_y == corner_y[i])
			{
				c->direct = corner_test(i, c->direct);
				break;
			}
		}
		if(i==corner_num)//not at corner 
			c->direct = turn(c->direct);
	}
	switch(c->direct)
	{
		case 0:
			post_x = pre_x + speed; 
			post_y = pre_y;
			break;
		case 1:
			post_x = pre_x; 
			post_y = pre_y - speed;
			break;
		case 2:
			post_x = pre_x - speed; 
			post_y = pre_y;
			break;
		case 3:
			post_x = pre_x; 
			post_y = pre_y + speed;
			break;
	}
	c->x = post_x;
	c->y = post_y;
}

int corner_test(int corner, int direct)
{
	//up:1 down:3 right:0 left:2
	switch(corner)
	{
		case 0:
			if(direct==2)
				return 3;
			else
				return 0;
		case 1:
			if(direct==0)
				return 3;
			else
				return 2;
		case 2:
			if(direct==0)
				return 1;
			else
				return 2;
		case 3:
			if(direct==2)
				return 1;
			else
				return 0;
	}
}

void compute_power()
{
	int i;
	struct car * c = t_list;	
	while(c!=NULL)
	{
		//compute the new power of four base station
		double dd;
		for(i=0 ; i<bs_num ; ++i)
		{
			dd = distance_d((double)c->x, (double)c->y, (double)bs_x[i], (double)bs_y[i]);
			if(dd <= 1.0)
				c->p[i] = -60;
			else
				c->p[i] = -60 - 20*log10(dd);
		}
		//decide hand off or not by four policy
		hand_off(c);
		
		c = c->next;
	}
}

void hand_off(struct car * c)
{
	int max_index, i;
	//policy 1: if p_new>p_old
	max_index = max_of_arr(c->p, bs_num);
	if(c->p[max_index] > c->p[c->current_bs[0]])
	{
		c->current_bs[0] = max_index;
		hang_off_num[0]++;
	}
	
	//policy 2: if p_new>p_old & p_old<-110
	if(c->p[max_index] > c->p[c->current_bs[1]] && c->p[c->current_bs[1]] < Threshold)
	{
		c->current_bs[1] = max_index;
		hang_off_num[1]++;
	}
	
	//policy 3: if p_new>p_old+5
	if(c->p[max_index] > c->p[c->current_bs[2]] && c->p[max_index] > c->p[c->current_bs[2]]+Entropy)
	{
		c->current_bs[2] = max_index;
		hang_off_num[2]++;
	}
	
	//policy 4: handoff per 125 sec
	if(c->p[max_index] > c->p[c->current_bs[3]])
	{
		if(c->time%handoff_per_turn == 0 || c->p[c->current_bs[1]] < -125)
		{
			c->current_bs[3] = max_index;
			hang_off_num[3]++;
		}
	}
		
	double p ;
	double p10;
	for(i=0 ; i<policy_num ; ++i)
	{
		p = c->p[c->current_bs[i]];
		p10 = p/10;
		total_power[i] += pow(10, p10);
	}
}

void car_out()
{
	struct car *t = NULL;
	struct car *p = NULL;
	struct car *n = NULL;
	
	if(t_list!=NULL)
	{
		t = t_list;
		while(t!=NULL)
		{
			n = t->next;
			if(t->x<min_xy || t->x>max_xy || t->y<min_xy || t->y>max_xy)
			{	
				if(p==NULL)
				{
					t_list = t->next;
					free(t);
				}
				else
				{
					p->next = t->next;
					free(t);
				}
			}
			else
			{
				p = t;
			}
			t = n;	
		}
	}	
}

int search_by_pos(int x, int y)
{
	if(t_list==NULL)
		return 0;
	else
	{
		struct car * t = t_list;
		while(t!=NULL)
		{
			if(t->x==x && t->y==y)
				return t->id;
			t = t->next;
		}
		return 0;
	}
}

int distance_i(int x1, int y1, int x2, int y2)
{
	return (int)sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

double distance_d(double x1, double y1, double x2, double y2)
{
	return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

int max_of_arr(double d[], int n)
{
	int index = 0, i;
	for(i=1 ; i<n ; ++i)
	{
		if(d[i] > d[index])
			index = i;
	}
	return index;
}

void print()
{
	int i, j, k ,u, have, id;
	
	printf("  ");
	for(i=0 ; i<=(max_xy/75) ; ++i)
		printf("%3d",i);
	printf("\n");
	for(i=min_xy ; i<=(max_xy/75) ; ++i)
	{
		printf("%2d ",i);
		for(j=min_xy ; j<=(max_xy/75) ; ++j)
		{
			have = 0;
			for(k=i*75; k<i*75+75; ++k)
			{		
				for(u=j*75; u<j*75+75; ++u)
				{
					if(search_by_pos(u,k)!=0)
					{
						id=search_by_pos(u,k);
						have = 1;
						break;
					}		
				}
				if(have == 1)
					break;		
			}
			if(have == 1)
				printf("%3d",id);
			else
				printf(" - ");	
		}
		printf("\n");
	}
	
	/*struct car * t = t_list;
	while(t!=NULL)
	{
		printf("id:%d x:%d y:%d direct:%d\n",t->id,t->x,t->y,t->direct);
		t = t->next;
	}*/ 
}
