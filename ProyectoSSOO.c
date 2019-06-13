#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include <gtk/gtk.h>
#define atoa(x) #x
#include <string.h>

GtkWidget *labels[50];
struct sched_param sp;
int procesos[500];
int lista_procesos[500];
char lista_nombre_procesos[500][1000];
char CPU[500][1000];
char MEM[500][1000];
char STAT[500][1000];
char TIME[500][1000];
char lista_nombre_procesos_pid[500][1000];
int borrados = 0;

gint delete_event( GtkWidget *widget, //Función para salir de app
                   GdkEvent  *event,
                   gpointer   data )
{
        gtk_main_quit ();
        return FALSE;
}

int GetCPUCount () { //Función que devuelve el número de CPUs
  int i , count =0; 
  cpu_set_t cs; 
  sched_getaffinity (0, sizeof (cs), &cs); 
  for (i = 0; i < sizeof (cs ); i++) 
     if ( CPU_ISSET (i , &cs)) 
        count ++; 
  return count ; 
}

void add_proc(GtkWidget *button, GtkWidget **datos){ //Añade el proceso elegido a la lista
    int i,j;
    int pid=0;
    int mismonombre=0;
    int existe=0;
    gchar * gproc = gtk_combo_box_text_get_active_text(datos[0]);
    if(gproc!=NULL){
	char proc[50] = "";
	strcat(proc,gproc);
	for(i=0;i<500;i++){
		if(lista_nombre_procesos_pid[i]!=NULL){
			for(j=0;j<50;j++){
				if(proc[j]==lista_nombre_procesos_pid[i][j]){
					if(j==0) mismonombre=1;
				}
				else mismonombre=0;
			}
			
			if(mismonombre==1){ 
				pid = lista_procesos[i];
			}
		}
	}
	for(i=0;i<500;i++){
		if(procesos[i]==pid) existe=1;
	}
	if(existe==0){
		int x=-1;
		i=0;
		while(x==-1){
			if(procesos[i]==NULL){
				x=i;
				procesos[i]=pid;
			}else i++;	
		}	
		crear_caja(datos[1],pid,i*10);
	}
    }		
}

void eliminar_proceso(GtkWidget *button, GtkWidget *entry){ //Elimina/detiene un proceso
    int i,x;
    gchar *ch_pid = gtk_entry_get_text(entry);
    if(atoi(ch_pid)!=0){
	int pid = atoi(ch_pid);
	char killPid[20] = "kill ";
	strcat(killPid,ch_pid);
	system(killPid);
	for(i=0;i<500;i++){
		if(procesos[i]==pid){
			x=i;
			procesos[i]=NULL;
			borrados++;
		}
	}
	for(i=0;i<10;i++){
		gtk_label_set_text(labels[x*10+i], "");
	}
    }
}

void esconder_proceso(GtkWidget *button, GtkWidget *entry){ //Borra un proceso de la lista 
    int i,x;                                               //Se puede volver a añadir más tarde
    gchar *ch_pid = gtk_entry_get_text(entry);
    if(atoi(ch_pid)!=0){
	int pid = atoi(ch_pid);
	for(i=0;i<500;i++){
		if(procesos[i]==pid){
			x=i;
			procesos[i]=NULL;
			borrados++;
		}
	}
	for(i=0;i<10;i++){
		gtk_label_set_text(labels[x*10+i], "");
	}
    }
}

void cambiarPlan(int pid, int n, int prio){ //Cambia la política de planificación 
	sp.sched_priority =prio;	    //Parámetro n=1:CFS, n=2:RR, n=3:FIFO
	if(n==1) sched_setscheduler(pid, SCHED_OTHER, &sp);
	else if(n==2) sched_setscheduler(pid, SCHED_RR, &sp);
	else if(n==3) sched_setscheduler(pid, SCHED_FIFO, &sp);
}

void getPlan(int policy, char *prioridad){ //Devuelve el la política de planifiación de un proceso
	switch( policy ) { 		   //Se almacena el nombre en la variable "prioridad"
			case SCHED_OTHER : 
				 strcpy(prioridad, "CFS");
				break;			
			case SCHED_RR : 
				 strcpy(prioridad, "RR");
				break;
			case SCHED_FIFO :
				 strcpy(prioridad, "FIFO");
				break;
		   }
}

int cambiarPrioridadEstatica(int pid, int n){ //Modifica la prioridad estática
	sp.sched_priority = n;
	sched_setparam(pid,&sp); 
	return sp.sched_priority;
}

void cambiar_afinidad(GtkWidget *button, GtkWidget **datos){ //Función que se ejecuta al activar
    int i;						     //o desactivar los botones de CPUs
    int n=-1;					     //modifica las cpus afines al proceso
    char afi[15] = "";
    char s[5];
    int numCPU = GetCPUCount ();
    gchar *ch_pid = gtk_entry_get_text(datos[numCPU]);
    if(atoi(ch_pid)!=0){
	int pid = atoi(ch_pid);
	cpu_set_t set; 
	CPU_ZERO(&set);
	for(i=0;i<20;i++)
		if(procesos[i]!=NULL&&procesos[i] == pid){
			n = (i*10)+5;
		}
	if(n!=-1){
		for(i=0;i<numCPU;i++){
			if(gtk_toggle_button_get_active(datos[i])){
				CPU_SET(i,&set);
			}
		}
		sched_setaffinity (pid, sizeof (set), &set);
	 
		for (i = 0; i < sizeof (set ); i++){ 
			if ( CPU_ISSET (i , &set)) {
				sprintf(s, "%d ", i+1);
				strcat(afi,s);
			}
		}
		gtk_label_set_text(labels[n], afi);
	}
    }
}

void cambiar_nice(GtkWidget *button, GtkWidget **datos){ //Cambia el valor de nice si la política 
    gchar *ch_prio = gtk_combo_box_text_get_active_text(datos[0]); //de planificación es CFS
    gchar *ch_pid = gtk_entry_get_text(datos[1]);
    if(atoi(ch_pid)!=0&&ch_prio!=NULL){
	char sprio[5];
	int pid = atoi(ch_pid);
	int prio = atoi(ch_prio);
	int i, n;
	for(i=0;i<20;i++)
		if(procesos[i] == pid) n = (i*10)+4;
	if(gtk_label_get_text(labels[n])[0]!='-' || gtk_label_get_text(labels[n])[1]!=NULL){
		setpriority(PRIO_PROCESS, pid, prio);
		sprintf(sprio, "%d", getpriority(PRIO_PROCESS,pid));
		gtk_label_set_text(labels[n], sprio);
	}
    }
}

void cambiar_prio_estatica(GtkWidget *button, GtkWidget **datos){ //Cambia la prioridad estática
    gchar *ch_prio = gtk_combo_box_text_get_active_text(datos[0]); //Si la política de 
    gchar *ch_pid = gtk_entry_get_text(datos[1]);			//planifiación es RR o FIFO
    if(ch_prio!=NULL&&atoi(ch_pid)!=0){	
	char sprio[5];
	int pid = atoi(ch_pid);
	int prio = atoi(ch_prio);
	int i, n;
	
	for(i=0;i<20;i++)
		if(procesos[i] == pid) n = (i*10)+3;
	if(gtk_label_get_text(labels[n])[0]!='0'){
		prio = cambiarPrioridadEstatica(pid,prio);
		sprintf(sprio, "%d", prio);
		gtk_label_set_text(labels[n], sprio);
	}
    }
}

void cambiar_planificador(GtkWidget *button, GtkWidget **datos){ //Cambia la política de 
    gchar *plan = gtk_combo_box_text_get_active_text(datos[0]); //planificación
    gchar *ch_pid = gtk_entry_get_text(datos[1]);
    if(plan!=NULL&&atoi(ch_pid)!=0){
	char ch_plan[5]="";
	int pid = atoi(ch_pid);
	int i, n;
	for(i=0;i<20;i++)
		if(procesos[i] == pid) n = (i*10)+1;
	if(plan[0]=='C'){
		cambiarPlan(pid,1,0);
		if(sched_getscheduler(pid)==0){
			gtk_label_set_text(labels[n+2], "0");
			if(gtk_label_get_text(labels[n+3])[0]=='-') 
				gtk_label_set_text(labels[n+3], "0");
		}
	}
	else if(plan[0]=='R'){
		if(gtk_label_get_text(labels[n+2])[0]=='0'){
			cambiarPlan(pid,2,1);
			if(sched_getscheduler(pid)==2) gtk_label_set_text(labels[n+2], "1");

		}else cambiarPlan(pid,2,gtk_label_get_text(labels[n+2])[0]);
		gtk_label_set_text(labels[n+3], "-");
	}
	else if(plan[0]=='F'){
		if(gtk_label_get_text(labels[n+2])[0]=='0'){
			cambiarPlan(pid,3,1);
			if(sched_getscheduler(pid)==1) gtk_label_set_text(labels[n+2], "1");

		}else cambiarPlan(pid,3,gtk_label_get_text(labels[n+2])[0]);
		gtk_label_set_text(labels[n+3], "-");
	}
	getPlan(sched_getscheduler(pid), ch_plan);
	gtk_label_set_text(labels[n+1], ch_plan);
    }	
}

void crear_caja(GtkWidget *box1, int pid, int n){ //Función donde se crean/imprimen las líneas
	GtkWidget *box2;			  //con el proceso y su información
	GtkWidget *label;			  //Es todo código de la interfaz
        char spid[15];
	char sprio[15];
	char afi[15]="";
	char s[10];
        sprintf(spid, "%d", pid);
	char planificador[5]="";
	int i,j;
	int numCPU = GetCPUCount ();
	char nombre[50]="";
	char scpu[50];
	char smem[50];
	char sestado[50];
	char stiempo[50];
	int espacio = 0;
	sprintf(sprio, "%d", getpriority(PRIO_PROCESS,pid));
        getPlan(sched_getscheduler (pid),planificador);

	for(i=0;i<numCPU;i++){
		sprintf(s, "%d ", i+1);
		strcat(afi,s);	
	}
	for(i=0;i<500;i++){
		if(lista_procesos[i]==pid){
			for(j=0;j<50;j++){
				if(lista_nombre_procesos[i][j]==10) espacio=1;
				if(espacio==1&&j<20) nombre[j]=' ';
				else nombre[j] = lista_nombre_procesos[i][j];
				scpu[j] = CPU[i][j];
				smem[j]=MEM[i][j];
				sestado[j]=STAT[i][j];
				stiempo[j]=TIME[i][j];
			} 
		}
	}
       	if(borrados==0){         
        	box2 = gtk_hbox_new (FALSE, 0);
        	labels[n] = gtk_label_new(nombre);
        	gtk_box_pack_start (GTK_BOX (box2), labels[n], FALSE, FALSE, 0);
        	gtk_widget_show (labels[n]);
        	labels[n+1] = gtk_label_new(spid);
        	gtk_box_pack_start (GTK_BOX (box2), labels[n+1], TRUE, FALSE, 0);
        	gtk_widget_show (labels[n+1]);
        	labels[n+2] = gtk_label_new(planificador);
        	gtk_box_pack_start (GTK_BOX (box2), labels[n+2], TRUE, TRUE, 0);
        	gtk_widget_show (labels[n+2]);
		if(planificador[0]=='C') labels[n+3] = gtk_label_new("0");
		else labels[n+3] = gtk_label_new("1");
        	gtk_box_pack_start (GTK_BOX (box2), labels[n+3], TRUE, TRUE, 0);
        	gtk_widget_show (labels[n+3]);
		if(planificador[0]=='C') labels[n+4] = gtk_label_new(sprio);
		else labels[n+4] = gtk_label_new("-");
        	gtk_box_pack_start (GTK_BOX (box2), labels[n+4], TRUE, TRUE, 0);
        	gtk_widget_show (labels[n+4]);
        	labels[n+5] = gtk_label_new(afi);
        	gtk_box_pack_start (GTK_BOX (box2), labels[n+5], TRUE, TRUE, 0);
        	gtk_widget_show (labels[n+5]);
        	labels[n+6] = gtk_label_new(scpu);
        	gtk_box_pack_start (GTK_BOX (box2), labels[n+6], TRUE, TRUE, 0);
        	gtk_widget_show (labels[n+6]);
        	labels[n+7] = gtk_label_new(smem);
        	gtk_box_pack_start (GTK_BOX (box2), labels[n+7], TRUE, TRUE, 0);
        	gtk_widget_show (labels[n+7]);
        	labels[n+8] = gtk_label_new(sestado);
        	gtk_box_pack_start (GTK_BOX (box2), labels[n+8], TRUE, TRUE, 0);
        	gtk_widget_show (labels[n+8]);
        	labels[n+9] = gtk_label_new(stiempo);
        	gtk_box_pack_start (GTK_BOX (box2), labels[n+9], TRUE, TRUE, 0);
        	gtk_widget_show (labels[n+9]);
		gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
		gtk_widget_show (box2);
	}else{
		gtk_label_set_text(labels[n], nombre);
		gtk_label_set_text(labels[n+1], spid);
		gtk_label_set_text(labels[n+2], planificador);
		if(planificador[0]=='C') gtk_label_set_text(labels[n+3], "0");
		else gtk_label_set_text(labels[n+3], "1");
		if(planificador[0]=='C') gtk_label_set_text(labels[n+4],sprio);
		else gtk_label_set_text(labels[n+4],"-");
		gtk_label_set_text(labels[n+5], afi);
		gtk_label_set_text(labels[n+6], scpu);
		gtk_label_set_text(labels[n+7], smem);
		gtk_label_set_text(labels[n+8], sestado);
		gtk_label_set_text(labels[n+9], stiempo);
		borrados--;
	}	
}

void nombres_variables(GtkWidget *box1){ //Se imprime la línea de arriba de los procesos
	GtkWidget *box2;		//Donde aparecen los nombres de las variables
	GtkWidget *label;		//Código de la interfaz
        box2 = gtk_hbox_new (FALSE, 0);
        	label = gtk_label_new("Nombre proceso  ");
        	gtk_box_pack_start (GTK_BOX (box2), label, FALSE, FALSE, 0);
        	gtk_widget_show (label);
        	label = gtk_label_new("PID  ");
        	gtk_box_pack_start (GTK_BOX (box2), label, TRUE, FALSE, 0);
        	gtk_widget_show (label);
        	label = gtk_label_new("Planificador  ");
        	gtk_box_pack_start (GTK_BOX (box2), label, TRUE, TRUE, 0);
        	gtk_widget_show (label);
        	label = gtk_label_new("Prio estática  ");
        	gtk_box_pack_start (GTK_BOX (box2), label, TRUE, TRUE, 0);
        	gtk_widget_show (label);
        	label = gtk_label_new("Nice  ");
        	gtk_box_pack_start (GTK_BOX (box2), label, TRUE, TRUE, 0);
        	gtk_widget_show (label);
        	label = gtk_label_new("CPUs afines  ");
        	gtk_box_pack_start (GTK_BOX (box2), label, TRUE, TRUE, 0);
        	gtk_widget_show (label);
        	label = gtk_label_new("%CPU  ");
        	gtk_box_pack_start (GTK_BOX (box2), label, TRUE, TRUE, 0);
        	gtk_widget_show (label);
        	label = gtk_label_new("%Memoria  ");
        	gtk_box_pack_start (GTK_BOX (box2), label, TRUE, TRUE, 0);
        	gtk_widget_show (label);
        	label = gtk_label_new("Estado  ");
        	gtk_box_pack_start (GTK_BOX (box2), label, TRUE, TRUE, 0);
        	gtk_widget_show (label);
        	label = gtk_label_new("Tiempo  ");
        	gtk_box_pack_start (GTK_BOX (box2), label, TRUE, TRUE, 0);
        	gtk_widget_show (label);
		gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
		gtk_widget_show (box2);
}


void lista() //Se leen del proc los datos necesarios (los procesos en ejecución y sus propiedades)
{	     //Para esto se utiliza un fichero en el que se almecenará el resultado de la 
	FILE *fichero=NULL; //instrucción y con una variable se guarda el resultado y se pasa a la 
	char aux[500][1000]; //interfaz
	char aux1[500][1000];
	char aux2[500][1000];
	int i,j,k;

	fichero = popen ("ps -A | awk '{print $1}'", "r");
        i=0;
	fgets (aux1[i], 1000, fichero);
	i++;
	while (!feof (fichero))
	{	
		fgets (aux1[i], 1000, fichero);
		i++;
	}
	pclose (fichero);

	fichero = popen ("ps axu | awk '{print $2}'", "r");
        i=0;
	fgets (aux2[i], 1000, fichero);
	i++;
	while (!feof (fichero))
	{	
		fgets (aux2[i], 1000, fichero);
		i++;
	}
	j=i;
	i=0;
	for(i=0;i<j;i++){
		if(atoi(aux1[i])==atoi(aux2[i])){
			lista_procesos[i]=atoi(aux1[i]);
		}
        }
	fichero = popen ("ps -A | awk '{print $4}'", "r");
        i=0;
	fgets (aux[i], 1000, fichero);
	i++;
	while (!feof (fichero))
	{	
		fgets (aux[i], 1000, fichero);
		i++;
	}
	j=i;
	i=0;
	for(i=0;i<j;i++){
		if(atoi(aux1[i])==atoi(aux2[i])){
			for(k=0;k<50;k++){
				if(aux[k]!=NULL) lista_nombre_procesos[i][k]=aux[i][k];
			}
		}
        }
	pclose (fichero);

	fichero = popen ("ps axu | awk '{print $3}'", "r");
        i=0;
	fgets (aux[i], 1000, fichero);
	i++;
	while (!feof (fichero))
	{	
		fgets (aux[i], 1000, fichero);
		i++;
	}
	j=i;
	i=0;
	for(i=0;i<j;i++){
		if(atoi(aux1[i])==atoi(aux2[i])){
			for(k=0;k<50;k++){
				if(aux[k]!=NULL) CPU[i][k]=aux[i][k];
			}
		}
        }		
	pclose (fichero);

	fichero = popen ("ps axu | awk '{print $4}'", "r");
        i=0;
	fgets (aux[i], 1000, fichero);
	i++;
	while (!feof (fichero))
	{	
		fgets (aux[i], 1000, fichero);
		i++;
	}
	j=i;
	i=0;
	for(i=0;i<j;i++){
		if(atoi(aux1[i])==atoi(aux2[i])){
			for(k=0;k<50;k++){
				if(aux[k]!=NULL) MEM[i][k]=aux[i][k];
			}
		}
        }	
	pclose (fichero);

	fichero = popen ("ps axu | awk '{print $8}'", "r");
        i=0;
	fgets (aux[i], 1000, fichero);
	i++;
	while (!feof (fichero))
	{	
		fgets (aux[i], 1000, fichero);
		i++;
	}
	j=i;
	i=0;
	for(i=0;i<j;i++){
		if(atoi(aux1[i])==atoi(aux2[i])){
			for(k=0;k<50;k++){
				if(aux[k]!=NULL) STAT[i][k]=aux[i][k];
			}
		}
        }	
	pclose (fichero);
	fichero = popen ("ps axu | awk '{print $10}'", "r");
        i=0;
	fgets (aux[i], 1000, fichero);
	i++;
	while (!feof (fichero))
	{	
		fgets (aux[i], 1000, fichero);
		i++;
	}
	j=i;
	i=0;
	for(i=0;i<j;i++){
		if(atoi(aux1[i])==atoi(aux2[i])){
			for(k=0;k<50;k++){
				if(aux[k]!=NULL) TIME[i][k]=aux[i][k];
			}
		}
        }	
	pclose (fichero);

}


int main( int   argc,  //Main, donde se crean todos los botones y la interfaz en sí
          char *argv[])	//Es todo código de la interfaz
{
        GtkWidget *window;
        GtkWidget *button;
	GtkWidget *button1;
	GtkWidget *button2;
        GtkWidget *box1;
        GtkWidget *separator;
	GtkWidget *label;
        GtkWidget *quitbox;
        GtkWidget *entry;
	GtkWidget *entrybox;
	GtkWidget *entrybox1;
	GtkWidget *entrybox2;
	GtkWidget *combobox;
	GtkWidget *combobox1;
	GtkWidget *combobox2;
	GtkWidget *tbutton1;
	GtkWidget *tbutton2;
	GtkWidget *tbutton3;
	GtkWidget *tbutton4;
	GtkWidget *deletebox;
	GtkWidget *deletebutton;
	GtkWidget *hidebutton;
	GtkWidget *listabox;
	GtkWidget *lista_combobox;
	GtkWidget *button_addproc;
	char snum[2]="";
	int i,j;
	int numCPU = GetCPUCount ();

        gtk_init (&argc, &argv);
        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        g_signal_connect (G_OBJECT (window), "delete_event",
                          G_CALLBACK (delete_event), NULL);
        gtk_container_set_border_width (GTK_CONTAINER (window), 10);
        box1 = gtk_vbox_new (FALSE, 0);

//Título de la parte de arriba donde se añaden los procesos
        label = gtk_label_new ("Añadir procesos");
        gtk_box_pack_start (GTK_BOX (box1), label, FALSE, FALSE, 0);
        gtk_widget_show (label);
       
//Separador
       separator = gtk_hseparator_new ();
       gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 5);
       gtk_widget_show (separator);

//Lista donde elegir los procesos y botón de añadir procesos
	lista();
	char s_pid[10]="";
	listabox = gtk_hbox_new (FALSE, 0);
	lista_combobox = gtk_combo_box_text_new();
	for(i=0;i<500;i++){
		if(lista_procesos[i]!=NULL){
			sprintf(s_pid, "%d", lista_procesos[i]);
			for(j=0;j<50;j++){
				lista_nombre_procesos_pid[i][j] = lista_nombre_procesos[i][j];
			}
			strcat(lista_nombre_procesos_pid[i],"PID:"); 
			strcat(lista_nombre_procesos_pid[i],s_pid);
			gtk_combo_box_text_append_text(lista_combobox,lista_nombre_procesos_pid[i]);
		}
	}
	gtk_box_pack_start (GTK_BOX (listabox), lista_combobox, FALSE, FALSE, 0);
	gtk_widget_show (lista_combobox);

	GtkWidget *datos[2];
	datos[0]=lista_combobox;
	datos[1]=box1;

        button_addproc = gtk_button_new_with_label ("Añadir proceso");
        g_signal_connect(G_OBJECT (button_addproc), "clicked",
                                  G_CALLBACK (add_proc), datos);
        gtk_box_pack_start (GTK_BOX (listabox), button_addproc, FALSE, FALSE, 0);
	gtk_widget_show (button_addproc);
        gtk_box_pack_start (GTK_BOX (box1), listabox, FALSE, FALSE, 0);
	gtk_widget_show (listabox);


//Separador
       separator = gtk_hseparator_new ();
       gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 5);
       gtk_widget_show (separator);

//Título del apartado de acciones
        label = gtk_label_new ("Acciones");
        gtk_box_pack_start (GTK_BOX (box1), label, FALSE, FALSE, 0);
        gtk_widget_show (label);

//Separador
       separator = gtk_hseparator_new ();
       gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 5);
       gtk_widget_show (separator);

//Entrada para el PID del proceso al que se va a acceder
	entrybox = gtk_hbox_new (FALSE, 0);
        label = gtk_label_new ("PID del proceso:");
        gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
        gtk_box_pack_start (GTK_BOX (entrybox), label, FALSE, FALSE, 0);
        gtk_widget_show (label);
	entry = gtk_entry_new();
        gtk_box_pack_start (GTK_BOX (entrybox), entry, FALSE, FALSE, 0);
        gtk_widget_show (entry);

//Lista con las políticas de planifiación y botón para cambiarlas
	combobox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(combobox,"CFS");
	gtk_combo_box_text_append_text(combobox,"RR");
	gtk_combo_box_text_append_text(combobox,"FIFO");
	gtk_box_pack_start (GTK_BOX (entrybox), combobox, FALSE, FALSE, 0);
	gtk_widget_show (combobox);
	
	GtkWidget * multidatos[2];
	multidatos[0] = combobox;
	multidatos[1] = entry;

        button = gtk_button_new_with_label ("Cambiar planificador");
        g_signal_connect(G_OBJECT (button), "clicked",
                                  G_CALLBACK (cambiar_planificador), multidatos);
        gtk_box_pack_start (GTK_BOX (entrybox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (box1), entrybox, FALSE, FALSE, 0);
	gtk_widget_show (entrybox);

//Lista con las prioridades estática y nice y botones para cambiarlas
	entrybox1 = gtk_hbox_new (FALSE, 0);
        label = gtk_label_new ("Prioridad estática");
        gtk_box_pack_start (GTK_BOX (entrybox1), label, FALSE, FALSE, 0);
        gtk_widget_show (label);
	combobox1 = gtk_combo_box_text_new();
	
	for(i=1;i<100;i++){
		sprintf(snum, "%d", i);
		gtk_combo_box_text_append_text(combobox1,snum);
	}	
	
	gtk_box_pack_start (GTK_BOX (entrybox1), combobox1, FALSE, FALSE, 0);
	gtk_widget_show (combobox1);

	GtkWidget * multidatos1[2];
	multidatos1[0] = combobox1;
	multidatos1[1] = entry;


        button1 = gtk_button_new_with_label ("Cambiar");
        g_signal_connect(G_OBJECT (button1), "clicked",
                                  G_CALLBACK (cambiar_prio_estatica), multidatos1);
        gtk_box_pack_start (GTK_BOX (entrybox1), button1, FALSE, FALSE, 0);
	gtk_widget_show (button1);

        label = gtk_label_new ("Nice");
        gtk_box_pack_start (GTK_BOX (entrybox1), label, FALSE, FALSE, 0);
        gtk_widget_show (label);
	combobox2 = gtk_combo_box_text_new();
	
	for(i=-20;i<20;i++){
		sprintf(snum, "%d", i);
		gtk_combo_box_text_append_text(combobox2,snum);
	}	
	
	gtk_box_pack_start (GTK_BOX (entrybox1), combobox2, FALSE, FALSE, 0);
	gtk_widget_show (combobox2);

	GtkWidget * multidatos2[2];
	multidatos2[0] = combobox2;
	multidatos2[1] = entry;

        button2 = gtk_button_new_with_label ("Cambiar");
        g_signal_connect(G_OBJECT (button2), "clicked",
                                  G_CALLBACK (cambiar_nice), multidatos2);
        gtk_box_pack_start (GTK_BOX (entrybox1), button2, FALSE, FALSE, 0);
	gtk_widget_show (button2);
        gtk_box_pack_start (GTK_BOX (box1), entrybox1, FALSE, FALSE, 0);
	gtk_widget_show (entrybox1);


//Texto y botones para cambiar la afinidad
	entrybox2 = gtk_hbox_new (FALSE, 0);

        label = gtk_label_new ("Afinidad: ");
        gtk_box_pack_start (GTK_BOX (entrybox2), label, FALSE, FALSE, 0);
        gtk_widget_show (label);

	tbutton1 = gtk_toggle_button_new_with_label("CPU1");
        gtk_box_pack_start (GTK_BOX (entrybox2), tbutton1, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(tbutton1,TRUE);
	gtk_widget_show (tbutton1);

	if(numCPU>1){
		tbutton2 = gtk_toggle_button_new_with_label("CPU2");
       		gtk_box_pack_start (GTK_BOX (entrybox2), tbutton2, FALSE, FALSE, 0);
		gtk_toggle_button_set_active(tbutton2,TRUE);
		gtk_widget_show (tbutton2);
	}
	if(numCPU>2){
		tbutton3 = gtk_toggle_button_new_with_label("CPU3");
       		gtk_box_pack_start (GTK_BOX (entrybox2), tbutton3, FALSE, FALSE, 0);
		gtk_toggle_button_set_active(tbutton3,TRUE);
		gtk_widget_show (tbutton3);
	}
	if(numCPU>3){
		tbutton4 = gtk_toggle_button_new_with_label("CPU4");
       		gtk_box_pack_start (GTK_BOX (entrybox2), tbutton4, FALSE, FALSE, 0);
		gtk_toggle_button_set_active(tbutton4,TRUE);
		gtk_widget_show (tbutton4);
	}

	GtkWidget *multidatos3[numCPU+1];
	for(i=0;i<numCPU;i++){
		if(i==0) multidatos3[0]=tbutton1;
		if(i==1) multidatos3[1]=tbutton2;
		if(i==2) multidatos3[2]=tbutton3;
		if(i==3) multidatos3[3]=tbutton4;
	}
	multidatos3[numCPU]=entry;

	g_signal_connect(G_OBJECT (tbutton1), "toggled", G_CALLBACK (cambiar_afinidad), multidatos3);
	if(numCPU>1) g_signal_connect(G_OBJECT (tbutton2), "toggled", G_CALLBACK (cambiar_afinidad), multidatos3);
	if(numCPU>2) g_signal_connect(G_OBJECT (tbutton3), "toggled", G_CALLBACK (cambiar_afinidad), multidatos3);
	if(numCPU>3) g_signal_connect(G_OBJECT (tbutton4), "toggled", G_CALLBACK (cambiar_afinidad), multidatos3);

        gtk_box_pack_start (GTK_BOX (box1), entrybox2, FALSE, FALSE, 0);
	gtk_widget_show (entrybox2);
	
//Botones de eliminar y esconder proceso
        deletebox = gtk_hbox_new (FALSE, 0);

        deletebutton = gtk_button_new_with_label ("Eliminar proceso");
        g_signal_connect(G_OBJECT (deletebutton), "clicked",
                                  G_CALLBACK (eliminar_proceso), entry);
        gtk_box_pack_start (GTK_BOX (deletebox), deletebutton, FALSE, FALSE, 0);
	gtk_widget_show (deletebutton);

        hidebutton = gtk_button_new_with_label ("Esconder proceso");
        g_signal_connect(G_OBJECT (hidebutton), "clicked",
                                  G_CALLBACK (esconder_proceso), entry);
        gtk_box_pack_start (GTK_BOX (deletebox), hidebutton, FALSE, FALSE, 0);
	gtk_widget_show (hidebutton);

        gtk_box_pack_start (GTK_BOX (box1), deletebox, FALSE, FALSE, 0);
	gtk_widget_show (deletebox);

//Separador
       separator = gtk_hseparator_new ();
       gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 5);
       gtk_widget_show (separator);

//Título del último apartado, donde va la lista de procesos elegidos
        label = gtk_label_new ("Lista de procesos");
        gtk_box_pack_start (GTK_BOX (box1), label, FALSE, FALSE, 0);
        gtk_widget_show (label);
       
//Separador
       separator = gtk_hseparator_new ();
       gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 5);
       gtk_widget_show (separator);
	
	nombres_variables(box1);	
 
        gtk_container_add (GTK_CONTAINER (window), box1);
        gtk_widget_show (box1);
        gtk_widget_show (window);
        gtk_main ();

        return 0;
}




//Compilar proyecto: sudo gcc ProyectoSSOO.c -o ProyectoSSOO $(pkg-config gtk+-3.0 --cflags --libs)




