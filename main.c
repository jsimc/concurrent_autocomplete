#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>


typedef struct trie_node{
    char c;
    int term;
    int subwords;
    struct trie_node *parent;
    struct trie_node *children[26];
} trie_node;

typedef struct scanned_file {
    char file_name[64];
    time_t mod_time;
} scanned_file;

typedef struct search_result {
    int result_count;
    char **words;
} search_result;

//FUNKCIJE
void make_token_array(char *token_input[2], char* str);
trie_node* trie_init();
trie_node* new_trie_node(trie_node* parent, char slovo);
void add_subwords_rec(trie_node *child);
void trie_add_word(char *key);
void suggestion_recursive(trie_node *curr, char *curr_prefix, search_result *ans, int *brojac_ans);
                                            //moguce da ti nece trebati ovo kao argument funkcije jer ce verovatno biti
                                            //globalna promenljiva, jer jedan main_thread radi sa tim.
void trie_free_result(search_result *ans);
search_result* trie_get_words(char *query);
void* rad_sa_folderom(void *args_);
char* path_name(char *entry_name, char *name_of_directory, int lenOfNameDir);
time_t return_mod_time(char *full_path);
time_t rad_sa_datotekom(char *full_path);
void to_lowercase(char *str);
int only_letters(char *word);
int is_letter(char c);
void printTrie(trie_node* node);
//void* waiting_fun(void *args_);

//GLOBALNE
trie_node *head;
pthread_t scanner_threads[100];
//char *args_for_scanner_thread[100];
int num_of_scanner_threads = 0;
pthread_mutex_t mutexes[26];
char *last_prefix;
int argument = 0; //cekamo da mi on bude -1 da znam da je uhvatio ctrl+d
int komanda_mod;
/*
scanned_file skenirane_datoteke[1000];
int brojac_skeniranih =0;
*/

int main() {
    head = trie_init();
    char *input = malloc(sizeof(char*)*64);
    char *isNull = malloc(sizeof(char*)*64);
    char *token_input[2];
    for(int i = 0; i < 2; i++) {
        token_input[i] = malloc(64*sizeof(char));
    }
    for(int i = 0; i < 26; i++) {
        pthread_mutex_init(&mutexes[i], NULL);
    }

    komanda_mod = 1;

    while(1) {
        for(int i = 0; i < 2; i++) {
            token_input[i] = malloc(64*sizeof(char));
        }
        /*
        if(komanda_mod) {
            printf("Unesite komandu: \n");
            printf("1. _add_ <dir>\n");
            printf("2. _stop_\n");
            printf("3. <prefix>\n");
        } else {
            printf("Work in progress...\n");
            printf("For end type: CTRL+D\n");
        }*/
        
        //printf("waiting...\n");
        //isNull = fgets(input, sizeof(char*)*64, stdin);
        //input[strlen(input)-1] = 0;
        //printf("input: %s; isNull: %s\n", input, isNull);

        //make_token_array(token_input, input);
        
        if(komanda_mod) {
            printf("Unesite komandu: \n");
            printf("1. _add_ <dir>\n");
            printf("2. _stop_\n");
            printf("3. <prefix>\n");

            fgets(input, sizeof(char*)*64, stdin);
            input[strlen(input)-1] = 0;
            make_token_array(token_input, input);

            if(strcmp(token_input[0], "_add_") == 0) {
                //printf("token_input[0] = %s; token_input[1] = %s\n", token_input[0], token_input[1]); 
                if(token_input[1] == NULL) {
                    printf("Nije uneto ime direktorijuma.\n");
                    continue;
                }

                //strcpy(args_for_scanner_thread[num_of_scanner_threads], token_input[1]);
                //printf("token_input[1] = %s\n", token_input[1]); 
                char *arg = calloc(sizeof(char*), 64);
                strcpy(arg, token_input[1]);
                pthread_create(scanner_threads+num_of_scanner_threads, NULL, rad_sa_folderom, (void*)(arg));
                num_of_scanner_threads++;
                //free(arg);
                //rad_sa_folderom(token_input[1], head);
                //printf("obavljen rad sa folderom\n");
            }
            else if(strcmp(token_input[0], "_stop_") == 0) {
                for(int i = 0; i < 26; i++) {
                    pthread_mutex_destroy(&mutexes[i]);
                }
                printf("Gasi se aplikacija.\n");
                return 0;
            } else {
                //printTrie(head);
                komanda_mod =0;
                last_prefix = calloc(sizeof(char*), 64);
                strcpy(last_prefix, token_input[0]);
                //last_prefix[strlen(input)-1] = 0;

                search_result *ans = trie_get_words(input);
                
                
                if(ans != NULL) {
                    printf("ans_result_count: %d\n", ans->result_count);
                    for(int i = 0; i < ans->result_count; i++) {
                        if(ans->words[i]) {
                            printf("ans->word[i]: %s\n", ans->words[i]);
                        }
                    }
                    trie_free_result(ans);
                }
                char pom[10];
                printf("waiting for ctrl+D...\n");
                do {
                    isNull = fgets(pom, 10, stdin);
                } while (isNull != NULL);
                
                printf("docekao Ctrl+D!!!\n");
                komanda_mod = 1;
                last_prefix = NULL;
            }
        }

        for(int i = 0; i < 2; i++) {
            free(token_input[i]);
        }
    }

}

void make_token_array(char *token_input[2], char* str) {
    //char *tmp = str;
    char *token = strtok(str, " ");
    int i = 0;
    while(token != NULL) {
        if(i >= 2) {
            break;
        }
        strcpy(token_input[i++], token);
        token = strtok(NULL, " ");
    }
}


//TRIE_UTILS
//////////////////////////////////////////////////////////
trie_node* trie_init() {
    trie_node *head = malloc(sizeof(trie_node));
    head->c = '_'; //da li ovo moze ovako?, moze
    head->term = 0;
    head->subwords = 0;
    head->parent = NULL;
    for(int i = 0; i < 26; i++) {
        head->children[i] = NULL;
    }
    return head;
}

trie_node* new_trie_node(trie_node *parent, char slovo) {
    trie_node *node = malloc(sizeof(trie_node));
    node->parent = parent;
    node->subwords = 0;
    node->term = 0;
    node->c = slovo;
    for(int i = 0; i < 26; i++) {
        node->children[i] = NULL;
    }
    return node;
}

void add_subwords_rec(trie_node *child) {
    trie_node *tmp = child;
    while(tmp->parent != NULL) {
        //int index = tmp->parent->c - 'a';
        //pthread_mutex_lock(&mutexi[index]);
        tmp->parent->subwords++;
        tmp = tmp->parent;
        //pthread_mutex_unlock(&mutexi[index]);
    }
}

void trie_add_word(char *key) {
    trie_node *curr = head;
    pthread_mutex_lock(&mutexes[key[0]-'a']);
    for(int level = 0; level < strlen(key); level++) {
        int index = *(key+level)-'a';
        if (curr->children[index] == NULL) {
            curr->children[index] = new_trie_node(curr, *(key+level));
        }
        curr = curr->children[index];
        //printf("slovo: %c, roditelj = %c\n", *(key+level), curr->parent->c);
    }
    if(curr->term == 0 && komanda_mod == 0 && strncmp(last_prefix, key, strlen(last_prefix)) == 0) {
        printf("dodatno: %s\n", key);
    }
    curr->term = 1;
    add_subwords_rec(curr); //treba da doda broj podreci u podstablu za sve roditelje
    
    pthread_mutex_unlock(&mutexes[key[0]-'a']);
}

void suggestion_recursive(trie_node *curr, char* curr_prefix, search_result *ans, int *brojac_ans) {

    if(curr->term) {
        strcpy(ans->words[*brojac_ans], curr_prefix);
        *brojac_ans = *brojac_ans+1;
    }

    

    for(int i = 0; i < 26; i++) {
        if(curr->children[i]) {
            char child = 'a' + i;
            char *new_prefix = calloc(sizeof(char*)*64+sizeof(char)+1, 1); //? jel ovde puca
            int len = strlen(curr_prefix);
            strcpy(new_prefix, curr_prefix);
            new_prefix[len] = child;
            new_prefix[len+1] = '\0';


            suggestion_recursive(curr->children[i], new_prefix, ans, brojac_ans);
        }
        
        
    }
}

void trie_free_result(search_result *ans) {
    for(int i = 0; i < ans->result_count; i++) {
        free(ans->words[i]);
        ans->words[i] = NULL;
    }
    ans->result_count = 0;
    free(ans->words);
    ans->words = NULL;
    free(ans);
    ans = NULL;
}

/*void */
//Ideja mi je ovde da trie_get_words ne vraca nista, nego da ispise odma search_ans
//i da i ona bude u while(1) petlji, dokle god se ne unese ctrl+d, e sad za to mislila sam da 
//postoji neka druga nit koja ce da ceka ctrl+d
//a da while petlja takodje proverava da li je brojac == curr->result_count
//brojac ce da bude zapravo dokle smo stigli u search_result, tako da cemo njega i dalje prosledjivati
//on treba uvek da se na kraju popenje do ans_count, tj dokle god se nesto dodaje

//CEKAJ BANETOV MEJL

search_result* trie_get_words(char *query) {
    trie_node *curr = head;
    search_result *ans = malloc(sizeof(search_result*));

    char *isCtrlD = calloc(sizeof(char*), 64);
    char buf[10];
    int brojac = 0;
    //int argument = 0;
    //LOK NA PRVOM SLOVU
    pthread_mutex_lock(&mutexes[query[0]-'a']); //npr query[0] = 's', 's' - 'a' = 18
    for(int i = 0; i < strlen(query); i++) {
        int index = *(query+i) - 'a';

        if(curr->children[index] == NULL) {
            //puci ce ako se unese bilo sta sto nije malo slovo ali nema veze
            trie_free_result(ans);
            pthread_mutex_unlock(&mutexes[query[0]-'a']);
            return NULL; //no string in the trie has this prefix
        }

        curr = curr->children[index];
    }

    if(curr->term) {
        //printf("JESTE!\n");
        ans->result_count = curr->subwords+1;
    } else {
        ans->result_count = curr->subwords;
    }

    //printf("subwords of %c = %d\n", curr->c, curr->subwords);
    //printf("ans->result_count = %d\n", ans->result_count);
    ans->words = calloc(sizeof(char*), ans->result_count);
    for(int i = 0; i < ans->result_count; i++) {
        *(ans->words+i) = malloc(sizeof(char)*64);
    }
    

    suggestion_recursive(curr, query, ans, &brojac);
    //UNLOK NA PRVOM SLOVU
    pthread_mutex_unlock(&mutexes[query[0]-'a']);

    /*
    printf("brojac = %d; ans->result_count = %d\n", brojac, ans->result_count);
    if(brojac == ans->result_count) {
        printf("true; brojac= %d\n", brojac);
        //OVAJ BROJAC CE TI ZAPRAVO BITI KORISTAN
        //POSTO KADA KORISNIK UNESE REC KOJU ZELI DA PRETRAZI
        //DOK NE KLIKNE CTRL+D, NOVO NADJENE RECI IZ TOG PODSTABLA TREBAJU DA SE ISPISUJU
        //MADA NE MORAM NA TAJ NACIN
    }
    */
    //komanda_mod = 1;
    return ans; //return;
}

int isLeaf(trie_node *node) {
    for(int i = 0; i < 26; i++) {
        if(node->children[i]) {
            return 0;
        }
    }
    return 1;
}

/**moja pomocna funkcija za debag**/ 
void printTrie(trie_node *curr) {

    printf("c: %c\n", curr->c);
    if(curr->term) {
        printf("term\n");
    }

    for(int i = 0; i < 26; i++) {
        if(curr->children[i]) {
            //printf("c: %c\n", curr->c);


            printTrie(curr->children[i]);
        }
        
        
    }
}
/////////////////////////////////////////////////////////

//FILE_UTILS
/////////////////////////////////////////////////////////
void* rad_sa_folderom(void *args_) {
    //args_ mi je zapravo samo char* ime direktorijuma
    char *name_of_directory = calloc(sizeof(char*), 64);
    strcpy(name_of_directory, (char*)args_);

    printf("name of directory = %s\n", name_of_directory);

    DIR *directory = opendir(name_of_directory);
    int lenOfNameDir = strlen(name_of_directory);

    if(directory == NULL) {
        printf("Ne moze da se otvori direktorijum. \n");
        pthread_exit(0);
        //return -1;
    }

    scanned_file skenirane_datoteke[1000];
    int brojac_skeniranih =0;

    int prvi_put = 1;
    while(1) {
        struct dirent *entry;
        if(prvi_put) {
            while((entry = readdir(directory))) {
                if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                if(entry->d_type != 8) {
                    continue;
                }
                char *full_path = path_name(entry->d_name, name_of_directory, lenOfNameDir);

                //time_t last_mod_time = return_mod_time(full_path);
                //if(last_mod_time != skenirane_datoteke[brojac_skeniranih].mod_time) {
                skenirane_datoteke[brojac_skeniranih].mod_time = rad_sa_datotekom(full_path);
                //OVDE MI PUCA SEGM PROBLEM ---> JER NISI FILE_NAME STATICKI ALOCIRALAAAAAA
                strcpy(skenirane_datoteke[brojac_skeniranih].file_name, entry->d_name);
                brojac_skeniranih++;
                //}
            }
            printf("zavrseno prvi put\n");
            //sleep(5);
            prvi_put = 0;
            //closedir(directory);
            continue;
        }
        closedir(directory);
        directory = opendir(name_of_directory);

        int i = 0;
        while((entry = readdir(directory)) /*&& i < brojac_skeniranih*/) {
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
            if(entry->d_type != 8) {
                continue;
            }
            //printf("entry->name = %s\n", entry->d_name);
            char *full_path = path_name(entry->d_name, name_of_directory, lenOfNameDir);
            time_t last_mod_time = return_mod_time(full_path);
            int rez = strcmp(skenirane_datoteke[i].file_name, entry->d_name);
            if(rez == 0 && last_mod_time != skenirane_datoteke[i].mod_time) {
                printf("dodato %s\n", full_path); //RADIIIIIIII
                skenirane_datoteke[i].mod_time = rad_sa_datotekom(full_path);
                //rad sa datotekom bi trebalo da doda u trie
            }
            i++;
            free(full_path);
        }
        //closedir(directory);
       // sleep(5);
    }
}

char* path_name(char *entry_name, char *name_of_directory, int lenOfNameDir) {
    char *tmp = calloc(sizeof(name_of_directory) + 1 + 1 + sizeof(entry_name), 1);
                            //velicina imena direktorijuma + velicina karaktera '/' + 1 za '\0' + velicina imena fajla
    strcpy(tmp, name_of_directory);
    tmp[lenOfNameDir] = '/';
    strcat(tmp, entry_name);
    tmp[lenOfNameDir+strlen(entry_name)+1] = '\0';
    return tmp;
}

time_t rad_sa_datotekom(char *full_path) {
    FILE *f = fopen(full_path, "r");
    if(f == NULL) {
        printf("panikica: fopen\n");
        return -2L;
    }

    char *file_contents = calloc(sizeof(char*), 64);
    while((fscanf(f, "%s", file_contents)) != EOF) {
        if(!only_letters(file_contents)) {
            continue;
        }
        to_lowercase(file_contents);
        trie_add_word(file_contents);
    }

    fclose(f);

    return return_mod_time(full_path);
}

time_t return_mod_time(char *full_path) {
    struct stat sb;
    if (stat(full_path, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    return sb.st_mtime;
}

int only_letters(char *word) {
    for(int i = 0; i < strlen(word); i++) {
        if(!is_letter(*(word+i))) {
            return 0;
        }
    }
    return 1;
}

void to_lowercase(char *str) {
    for(int i = 0; i < strlen(str); i++) {
        if(*(str+i) >= 'A' && *(str+i) <= 'Z') {
            *(str+i) = *(str+i) + 32;
        }
    }
}

int is_letter(char c) {
    if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        return 1;
    }
    return 0;
}
/////////////////////////////////////////////////////////