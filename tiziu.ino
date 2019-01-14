#include <SoftwareSerial.h>
#include <SIM900.h>
#include <sms.h>
#include <EmonLib.h>
#include <DS3231.h>

#define voltage_calibration 600 //Valor de calibração ajustado com Multímetro

SMSGSM sms;
EnergyMonitor energy_monitor;
DS3231  _realTimeClock(SDA, SCL);  //Uno possui os pinos SDA e SCL conectores da porta digital

boolean is_gsm_ready = false;
int current_fase_one_pin = A1;
int voltage_fase_one_pin = A2;
int event_power_off_counter = 0;
int event_power_on_counter = 0;
int notifications_sent = 0;
char receiver[14] = "04111984184531";
String today = "";
String time_now = "";
String message_formater = "";
char message[110];

void greetings()
{
    Serial.println( F("    #####################################################"));
    Serial.println( F("    # - Eric Ikeda - Project Tiziu -       2019/01/12 - #"));
    Serial.println( F("    # - AC Voltage and Current Monitor - Version 0.01 - #"));
    Serial.println( F("    #####################################################"));
    delay(3000);
}

void setup() 
{
  Serial.begin(9600);
  greetings();

  _realTimeClock.begin();
  
  //Pino, calibracao - Cur Const= Ratio/BurdenR. 2000/330 = 6.0606
  energy_monitor.current(current_fase_one_pin, 6.0606);
  energy_monitor.voltage(voltage_fase_one_pin, voltage_calibration, 1.7);

  if (gsm.begin(9600)) 
  {
    Serial.println("nstatus=READY");
    is_gsm_ready = true;
  } 
}

void reset_counters()
{
  Serial.println("O fornecimento de energia foi restaurado.");
  event_power_off_counter = 0;
  event_power_on_counter = 0;
  notifications_sent = 0;
  Serial.println("Os contadores de eventos foram reiniciados.");
}

void format_message(char notification_message[110])
{
  today = _realTimeClock.getDateStr();
  time_now = _realTimeClock.getTimeStr();
  Serial.print("Data corrente: ");
  Serial.println(today);
  Serial.print("Hora atual: ");
  Serial.println(time_now);
  message_formater = today;
  message_formater += " - ";
  message_formater += time_now;
  message_formater += ": ";
  message_formater += String(notification_message);
  Serial.print("Mensagem que sera enviada: ");
  Serial.println(message_formater);

  message_formater.toCharArray(message, 110);
}

void send_notification(char notification_message[110])
{
  if(is_gsm_ready) 
  {
    format_message(notification_message);
    if (sms.SendSMS(receiver, message))
    {
      Serial.println("Notificacao via SMS enviada.");
      notifications_sent += 1;
    }
  }
  else
  {
    Serial.println("ATENCAO: Problemas de comunicacao com a rede celular.");
    Serial.println("Tentando enviar a mensagem novamente...");
    if (gsm.begin(9600)) 
    {
      Serial.println("nstatus=READY");
      is_gsm_ready = true;

      format_message(notification_message);
      if (sms.SendSMS(receiver, message))
      {
        Serial.println("Notificacao via SMS enviada.");
        notifications_sent += 1;
      }
    }
    else
    {
      Serial.println("ATENCAO: Notificacao nao enviada, verifique indisponibilidade da rede, saldo insuficiente (chip pre-pago), bloqueio do dispositivo, etc.");
    }
  }
}

void loop() 
{
  energy_monitor.calcVI(20,2000);
  
  float vrms = energy_monitor.Vrms;
  double irms = energy_monitor.calcIrms(1480);
  
  if(vrms > 110)
  {
    Serial.print("Tensao : ");
    Serial.print(vrms, 0);
    Serial.println(" V");

    if(event_power_off_counter > 0)
    {
      event_power_on_counter += 1;
      Serial.print("Obteve tensao valida na rede, leitura: ");
      Serial.print(event_power_on_counter);
      Serial.println(" de 2.");
      if(event_power_on_counter > 1) //Conta pelo menos duas leituras antes de notificar normalização da energia.
      {
        send_notification("O fornecimento de energia foi restaurado.");
        reset_counters();
      }
    }
  }
  else
  {
    event_power_off_counter += 1;
  }

  if(irms < 0.09)
  {
    Serial.println("Corrente : 0 A");
    Serial.println("Potencia : 0 W");    
  }
  else
  {
    Serial.print("Corrente : ");
    Serial.print(irms);
    Serial.println(" A");
  
    Serial.print("Potencia : ");
    Serial.print(irms * vrms);
    Serial.println(" W");
  }

  if(event_power_off_counter > 0)
  {
    Serial.print("Evento ");
    Serial.print(event_power_off_counter);
    Serial.println(" de 3.");
  }

  if(event_power_off_counter > 3 && notifications_sent < 3)
  {
    send_notification("ATENCAO: seu fornecimento de energia eletrica esta comprometido!");
  }

  delay(10000);
}