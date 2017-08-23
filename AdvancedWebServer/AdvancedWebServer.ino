#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#define FS_NO_GLOBALS
#include <FS.h> // include do sistema de arquivos SPI
 
namespace CONFIG{
  const char* ssid = "teste";
  const char* pass = "12345678";
  char temSPIFFS = 0;
  char nome[32] = "teste";
  char senha[32] = "12345678";
  char ledStatus=0;
}
 
ESP8266WebServer servidor;
namespace PAGINAS{


  String removeQuebra(String s){
    s.trim();
    return s;
  }
  
  void enviaArquivo(const char* nome){
    digitalWrite(D2,LOW);
    if(!CONFIG::temSPIFFS){ 
      servidor.send(200,"text/plain","SPIFFS nao inicializado!");
      digitalWrite(D2,HIGH);
      return;
    };
    fs::File arquivo=SPIFFS.open(nome,"r");
    servidor.setContentLength(CONTENT_LENGTH_UNKNOWN);
    servidor.streamFile(arquivo,"text/html");
    arquivo.close();
    digitalWrite(D2,HIGH);
  }

  void getData() {
    fs::File f = SPIFFS.open("/data.txt", "r");
    if (!f) {
        Serial.println("file open failed");
    }  
    String nome = PAGINAS::removeQuebra(f.readStringUntil('\n'));
    
    memset(CONFIG::nome,0,sizeof(CONFIG::nome));
    strncpy(CONFIG::nome,nome.c_str(),31);

    String senha = PAGINAS::removeQuebra(f.readStringUntil('\n'));
    
    memset(CONFIG::senha,0,sizeof(CONFIG::senha));
    strncpy(CONFIG::senha,senha.c_str(),31);

    f.close();
  }
    
  void handleIndex(){
    // envia o arquivo index.htm
    enviaArquivo("/index.htm");
  }
  
  void handleSetup(){
    // envia o arquivo setup.htm
    enviaArquivo("/setup.htm");
  }

  
  // Envia as configurações em formato JSON
  // no caso estou enviando só o nome e o estado do LED
  void handleDados(){
    PAGINAS::getData();
    servidor.setContentLength(CONTENT_LENGTH_UNKNOWN);
    String dadosJSON="";
    dadosJSON += "{";
    dadosJSON += "\"esp_nome\":\"";
    dadosJSON += strtok(CONFIG::nome,"\n");
    dadosJSON += "\",";
    dadosJSON += "\"esp_senha\":\"";
    dadosJSON += strtok(CONFIG::senha,"\n");
    dadosJSON += "\"}";
    servidor.send(200,"text/json",dadosJSON);
    servidor.client().stop();
    digitalWrite(D2,HIGH);
    
  }
  void handleConfig(){
    // descomentar para mostrar os argumentos recebidos pelo método POST
    /*int argCount = servidor.args();
    int i;
    for(i=0;i<argCount;i++){
      Serial.print(i);
      Serial.print(")");
      Serial.print(servidor.argName(i));
      Serial.print("=");
      Serial.println(servidor.arg(i));
    };*/
    if(servidor.hasArg("novo_nome")){
      String novo_nome = servidor.arg("novo_nome");
      String novo_senha = servidor.arg("novo_senha");

      fs::File f = SPIFFS.open("/data.txt", "w");
      if (!f) {
          Serial.println("file open failed");
      }
      f.println(novo_nome);
      f.println(novo_senha);
      f.close();
      
    };
    
    // Não sei se o certo seria fazer isso ou mandar
    // um header pra redirecionar o browser, mas isso
    // funciona também
    enviaArquivo("/index.htm");
  };
};
int pisca=0;
int pisca_dir=10;
void setup(){
  Serial.begin(115200);
  Serial.println(ESP.getResetReason()); // se o ESP foi resetado, a gente vai saber o motivo

  Serial.println("Inicializando SPIFFS");
  
 
  if(SPIFFS.begin()){  // inicializa o SPIFFS. por algum motivo o ESP reseta algunas vezes
                       // até essa função funcionar.
    CONFIG::temSPIFFS = 1;
    Serial.println("SPIFFS inicializado com sucesso!");
    Serial.println("Listando arquivos na memoria flash:");
    fs::Dir diretorio = SPIFFS.openDir("/");
    while(diretorio.next()){
      Serial.print(diretorio.fileName());
      Serial.print(" ");
      Serial.print(diretorio.fileSize());
      Serial.println(" bytes");
    };
    
    PAGINAS::getData();
    
  }else{
    CONFIG::temSPIFFS=0;
  };
 
  ESP.wdtEnable(8000); // configura o WDT pra resetar o ESP se ele ficar sem comer por 8 segundos
  pinMode(D1,OUTPUT);  // led vital. colocar um led amarelo aqui, catodo ligado no pino.
  pinMode(D2,OUTPUT);  // led de reset. colocar um led vermelho aqui com o catodo ligado no pino.
  pinMode(D3,OUTPUT);  // led que eu vou controlar. colocar um led verde aqui, catodo ligado no pino.
  digitalWrite(D2,LOW); // acende o led de reset pra gente saber que o ESP8266 reiniciou
 
  char * nome = strtok(CONFIG::nome, "\n");
  char * senha = strtok(CONFIG::senha, "\n");
  
  Serial.print("Conectando a ");
  Serial.print( nome );
  Serial.print( "/" );
  Serial.print( senha);
 
  WiFi.enableSTA(true); // coloca o ESP em modo estação (ele pode estar configurado como ponto de acesso)

  WiFi.begin(nome,senha);
 
  while(WiFi.status()!=WL_CONNECTED){
    delay(1000);
    Serial.println(WiFi.status());
    ESP.wdtFeed();
  };
  Serial.print("\nConectado! Meu IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("Inicializando servidor."); // configura o ESP para servir algumas páginas
  servidor.on("/" ,PAGINAS::handleIndex);
  servidor.on("/index" ,PAGINAS::handleIndex);
  servidor.on("/setup" ,PAGINAS::handleSetup);
  servidor.on("/config",PAGINAS::handleConfig);
  servidor.on("/dados", PAGINAS::handleDados);
 
  servidor.begin();
  Serial.println("Servidor inicializado");
  
  Serial.println("Esperando conexões...");
  // apaga o led de reset quando tudo estiver inicializado
  digitalWrite(D2,HIGH);
}
unsigned long old_millis;
void loop(){
  servidor.handleClient();
  // muda a intensidade do led vital
  // e liga e desliga o led D3 conforme a variável CONFIG::ledStatus
  if(millis() >= old_millis+10){
    pisca=pisca+pisca_dir;
    if(pisca>=1000)pisca_dir=-10;
    if(pisca<=10)  pisca_dir=10;
    analogWrite(D1,pisca);
    if(CONFIG::ledStatus) digitalWrite(D3,LOW); else digitalWrite(D3,HIGH);
    old_millis=millis();
  }
 
}
