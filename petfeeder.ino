//actuelle = V2.1: remplacement du RTC1307 par RTC3231 (pour eviter la dérive de l'horloge).
// Branchement RTC3231: 
// gnd=>gnd du arduino
// VCC=>5V du arduino
// SDA=>SDA du arduino
// SCL=>SCL du arduino
//old V2 : sens de rotation = horaire pour l'helice. Utilisation d'une helice avec pas à gauche et pas de vis = 50mm pour que les croquettes tombent plus facilement.
//old V1 : sens de rotation = anti-horaire pour l'helice. problème => la bouteille se dévisse et tombe.

#include <ds3231.h> // voir http://gilles.thebault.free.fr/spip.php?article53 pour explications
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <LCDKeypad.h> // from https://github.com/dzindra/LCDKeypad
#include <EEPROM.h>

//rappel : byte distributionSize  = (sizeof(distribution)/sizeof(int)); //array size

// ******** DEBUT VARIABLES ******** 
// On définit 10 emplacements en memoire soit 5 distrib/jour - doit tjrs etre pair. En théorie il y a 1022 emplacements possibles :
#define DISTRIB_MAX 10
// tableau qui contiendra les horaires de distrib :
int distribution[DISTRIB_MAX];
// initialisation horloge ds3231:
struct ts t;
//Initialisation variables programme
//La gamelle est-elle remplie : non
boolean GamelleRemplie = false;
// Gestion tours de l'helice de distribution :
byte TourPrecedent = 0;
// en 1/2 secondes, delta de temps pour déclencher le remplissage de la gamelle :
const byte TourDeclenchant = 4;
// pin Arduino pour le photorecepteur :
const byte PhotorecepteurPin = A3;
// pin Arduino pour le laser :
const byte LaserPin = 11;
//Declaration du servo helice :
Servo servo_helice;
// pin arduino du servo helice :
const byte Servo_helicePin = 2;
// vitesse de rotation du servo helice (0 à 180) :
// sens horaire : 65 | arret : 90 | sens anti-horaire : 115
const byte Vitesse_helice = 65;
//Declaration du servo clapet :
Servo servo_clapet;
// pin Arduino du servo clapet :
const byte Servo_clapetPin = 3;
//valeur haute du servo clapet (ouvert)
const byte Servo_ouvert = 90;
//valeur basse du servo clapet (fermé)
const byte Servo_ferme = 180;
// déclaration de l'écran :
LCDKeypad lcd;
// NOTA: Pin pour allumer/eteindre l'ecran = 10 par defaut
// button analog pin (default A0) - Lcd pins are 8, 9, 4, 5, 6, 7
// gestion de la veille pour l'écran :
boolean changement_etat = true;
boolean ecranAllume = true;
unsigned long endTime = 0;
// Nb de millisecondes d'inaction avant la mise en veille de l'écran (ici 1 min)
const long veille = 60000;
// caractères spéciaux :
//fleche haute
byte haut[8] = {
B00000,
B00100,
B01010,
B10001,
B00000,
B00000,
B00000,
B00000
};
//fleche basse
byte bas[8] = {
B00000,
B00000,
B00000,
B10001,
B01010,
B00100,
B00000,
B00000
};
//é
byte e_accent_aigu[8] = {
B00010,
B00100,
B01110,
B10001,
B11111,
B10000,
B01110,
B00000
};

/*Key Codes (in left-to-right order):

None   - KEYPAD_NONE
Select - KEYPAD_SELECT
Left   - KEYPAD_LEFT
Up     - KEYPAD_UP
Down   - KEYPAD_DOWN
Right  - KEYPAD_RIGHT
*/
// ******** FIN VARIABLES ******** 


// Fonction pour regler la mise à l'heure de l'horloge
void setRTC()
{
	//mise à l'heure
	lcd.clear();
	lcd.setCursor(5,0);
	lcd.print("R");
	lcd.write(byte(2));
	lcd.print("gler");
	lcd.setCursor(3,1);
	lcd.print("l");
	lcd.print((char)39);
	lcd.print("horloge ?");
	delay(2000);
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("< Oui      Non >");
	int buttonPressed;
	do
    {
      buttonPressed=waitButton();
    }
    while(!(buttonPressed==KEYPAD_LEFT || buttonPressed==KEYPAD_RIGHT));
    waitReleaseButton();
	if (buttonPressed==KEYPAD_LEFT) {
		byte value = 0;
		//int time_datas[2] = {0,0};

		lcd.clear();
		lcd.setCursor(1,0);
		lcd.print("Saisir l");
		lcd.print((char)39);
		lcd.print("heure");
		lcd.setCursor(3,1);
		lcd.write(byte(0));
		lcd.setCursor(5,1);
		lcd.print("00:00");
		lcd.setCursor(12,1);
		lcd.write(byte(1));
		while (!(buttonPressed==KEYPAD_SELECT)) {
			do
	   		{
		      buttonPressed=waitButton();
		    }
		    while(!(buttonPressed==KEYPAD_UP || buttonPressed==KEYPAD_DOWN || buttonPressed==KEYPAD_SELECT));
		    buttonPressed=waitButton();
		    if (buttonPressed==KEYPAD_UP) {
		    	value++;
				lcd.setCursor(5,1);
		    	if (value > 23)
					value = 0;
		    	if (value < 10)	{
		    		lcd.write('0');
		    		lcd.print(value,DEC);
		    	}
		    	else {
		    		lcd.print(value,DEC);
		    	}
		    }
		    if (buttonPressed==KEYPAD_DOWN) {
		    	value--;
				lcd.setCursor(5,1);
		    	if (value > 23 && value <= 255)
					value = 23;
		    	if (value < 10)	{
		    		lcd.write('0');
		    		lcd.print(value,DEC);
		    	}
		    	else {
		    		lcd.print(value,DEC);
		    	}
		    }
		    if (buttonPressed==KEYPAD_SELECT) {
		    	t.hour = value;
		    	value = 0;
				lcd.setCursor(0,0);
				lcd.print("    heure OK    ");
				delay(2000);
		    }
		}
		buttonPressed=KEYPAD_NONE;
		lcd.setCursor(0,0);
		lcd.print("puis les minutes");
		while (!(buttonPressed==KEYPAD_SELECT)) {
			do
	   		{
		      buttonPressed=waitButton();
		    }
		    while(!(buttonPressed==KEYPAD_UP || buttonPressed==KEYPAD_DOWN || buttonPressed==KEYPAD_SELECT));
		    buttonPressed=waitButton();
		    if (buttonPressed==KEYPAD_UP) {
		    	value++;
				lcd.setCursor(8,1);
		    	if (value > 59)
					value = 0;
		    	if (value < 10)	{
		    		lcd.write('0');
		    		lcd.print(value,DEC);
		    	}
		    	else {
		    		lcd.print(value,DEC);
		    	}
		    }
		    if (buttonPressed==KEYPAD_DOWN) {
		    	value--;
				lcd.setCursor(8,1);
		    	if (value > 59 && value <= 255)
					value = 59;
		    	if (value < 10)	{
		    		lcd.write('0');
		    		lcd.print(value,DEC);
		    	}
		    	else {
		    		lcd.print(value,DEC);
		    	}
		    }
		    if (buttonPressed==KEYPAD_SELECT) {
		    	t.min = value;
				lcd.setCursor(0,0);
				lcd.print("   minutes OK   ");
				delay(2000);
		    }
		}
		//DS3231_set(t) : commande a approfondir pour voir si on peut rentrer les valeurs individuellement
		t.sec=0;
  	t.mday=9;
  	t.mon=1;
  	t.year=2021;
		DS3231_set(t);		
	}

	if (buttonPressed==KEYPAD_RIGHT) {
		lcd.clear();
		lcd.home();
		lcd.print("R");
		lcd.write(byte(2));
		lcd.print("glage annul");
		lcd.write(byte(2));
		lcd.print("..");
		delay(1000);
	}
}

//Fonction pour afficher heure
void printTime()
{
  lcd.clear();
  lcd.setCursor(4,0);
  if (t.hour < 10)
    lcd.write('0');
  lcd.print(t.hour,DEC); // affiche l'heure
  lcd.print(":");
  if (t.min < 10)
    lcd.write('0');
  lcd.print(t.min,DEC); // affiche les minutes
  lcd.print(":");
  if (t.sec < 10)
    lcd.write('0');
  lcd.print(t.sec,DEC); // affiche les secondes
}

//Fonction pour réinitialiser les horaires de distribution
void razDistrib()
{
	//initialisation des distrib - ecranAllume = true;
	lcd.clear();
	lcd.setCursor(2,0);
	lcd.print("Horaires de");
	lcd.setCursor(3,1);
	lcd.print("repas par");
	delay(2000);
	lcd.clear();
	lcd.setCursor(3,0);
	lcd.print("repas par");
	lcd.setCursor(4,1);
	lcd.print("defaut ?");
	delay(2000);
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("< Oui      Non >");
	int buttonPressed;
	do
    {
      buttonPressed=waitButton();
    }
    while(!(buttonPressed==KEYPAD_LEFT || buttonPressed==KEYPAD_RIGHT));
    waitReleaseButton();
	if (buttonPressed==KEYPAD_LEFT) {
		int j [DISTRIB_MAX] = { 6,45,16,0,22,0 };
		for (byte i = 0; i < DISTRIB_MAX; i++) {
			if (i < 6) {
				EEPROM.update(i, j[i]);
			}
			// puis on complete le tableau avec la valeur 255
			else { EEPROM.update(i, 255); }
		}
		lcd.clear();
		lcd.home();
		lcd.print("RAZ effectu");
		lcd.write(byte(2));
		lcd.print("e...");
		delay(1000);
	}
	if (buttonPressed==KEYPAD_RIGHT) {
		lcd.clear();
		lcd.home();
		lcd.print("RAZ annul");
		lcd.write(byte(2));
		lcd.print("e.....");
		delay(1000);
	}
	chargeDistrib();
}

//Fonction qui attend une frappe sur un bouton et retourne sa valeur
int waitButton()
{
  int buttonPressed; 
  waitReleaseButton;
  while((buttonPressed=lcd.button())==KEYPAD_NONE) {}
  delay(100);  
  return buttonPressed;
}

void waitReleaseButton()
{
  delay(100);
  while(lcd.button()!=KEYPAD_NONE) {}
  delay(100);
}

//Fonction pour saisir ses propres horaires de distrib
void changeDistrib()
{
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("Cr");
	lcd.write(byte(2));
	lcd.print("er horaires ?");
	delay(2000);
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("< Oui      Non >");
	int buttonPressed;
	do
    {
      buttonPressed=waitButton();
    }
    while(!(buttonPressed==KEYPAD_LEFT || buttonPressed==KEYPAD_RIGHT));
    waitReleaseButton();
	if (buttonPressed==KEYPAD_LEFT) {
		byte numDistrib = 1;
		//on initialise toutes les valeurs du tableau distribution à la valeur 255
		for (byte i = 0; i < DISTRIB_MAX; i++) {
			distribution[i] = 255;
		}
		
		while ( numDistrib <= DISTRIB_MAX / 2 ) {
			lcd.clear();
			lcd.setCursor(0,0);
			lcd.print("Ajouter horaire");
			lcd.setCursor(6,1);
			lcd.write('N');
			lcd.print((char)223);
			lcd.write(' ');
			lcd.print(numDistrib, DEC);
			delay(2000);
			boolean ajoutOk = ajouterDistrib(numDistrib);
		    if (numDistrib < DISTRIB_MAX / 2 ) {
		    	lcd.clear();
				lcd.setCursor(3,0);
				lcd.print("Ajouter un");
				lcd.setCursor(5,1);
		  		lcd.print("horaire ?");
		  		delay(2000);
		  		lcd.clear();
				lcd.setCursor(0,0);
				lcd.print("< Oui      Non >");
				int buttonPressed;
				do
			    {
			      buttonPressed=waitButton();
			    }
			    while(!(buttonPressed==KEYPAD_LEFT || buttonPressed==KEYPAD_RIGHT));
			    waitReleaseButton();
				if (buttonPressed==KEYPAD_RIGHT) {
		  			//Si on a moins du maximum de distrib, on complete le tableau avec valeur 255.
		  			numDistrib = numDistrib * 2;
		  			for (byte i = numDistrib; i < DISTRIB_MAX; i++) {
		  				distribution[i] = 255;
		  			}
		  			break;
		  		}
		    }
		    if (ajoutOk == true)
		    	numDistrib++;
		}
		for (byte i = 0; i < DISTRIB_MAX; i++) {
			EEPROM.update(i, distribution[i]);
		}
	}
	if (buttonPressed==KEYPAD_RIGHT) {
		lcd.clear();
		lcd.home();
		lcd.print("Ajout annul");
		lcd.write(byte(2));
		lcd.print("....");
		delay(1000);
	}
	afficheHoraires();
	chargeDistrib();
}

//Fonction pour ajouter un horaire de distrib
boolean ajouterDistrib(byte param1)
{
	//Ajouter un horaire
	byte heure = 0;

	lcd.clear();
	lcd.setCursor(1,0);
	lcd.print("Saisir l");
	lcd.print((char)39);
	lcd.print("heure");
	lcd.setCursor(3,1);
	lcd.write(byte(0));
	lcd.setCursor(5,1);
	lcd.print("00:00");
	lcd.setCursor(12,1);
	lcd.write(byte(1));
	int buttonPressed;
		while (!(buttonPressed==KEYPAD_SELECT)) {
			do
	   		{
		      buttonPressed=waitButton();
		    }
		    while(!(buttonPressed==KEYPAD_UP || buttonPressed==KEYPAD_DOWN || buttonPressed==KEYPAD_SELECT));
		    buttonPressed=waitButton();
		    if (buttonPressed==KEYPAD_UP) {
		    	heure++;
				lcd.setCursor(5,1);
		    	if (heure > 23)
					heure = 0;
		    	if (heure < 10)	{
		    		lcd.write('0');
		    		lcd.print(heure,DEC);
		    	}
		    	else {
		    		lcd.print(heure,DEC);
		    	}
		    }
		    if (buttonPressed==KEYPAD_DOWN) {
		    	heure--;
				lcd.setCursor(5,1);
		    	if (heure > 23 && heure <= 255)
					heure = 23;
		    	if (heure < 10)	{
		    		lcd.write('0');
		    		lcd.print(heure,DEC);
		    	}
		    	else {
		    		lcd.print(heure,DEC);
		    	}
		    }
		    if (buttonPressed==KEYPAD_SELECT) {
				lcd.setCursor(0,0);
				lcd.print("    heure OK    ");
				delay(2000);
		    }
		}
		buttonPressed=KEYPAD_NONE;
		byte minute = 0;
		lcd.setCursor(0,0);
		lcd.print("puis les minutes");
		while (!(buttonPressed==KEYPAD_SELECT)) {
			do
	   		{
		      buttonPressed=waitButton();
		    }
		    while(!(buttonPressed==KEYPAD_UP || buttonPressed==KEYPAD_DOWN || buttonPressed==KEYPAD_SELECT));
		    buttonPressed=waitButton();
		    if (buttonPressed==KEYPAD_UP) {
		    	minute++;
				lcd.setCursor(8,1);
		    	if (minute > 59)
					minute = 0;
		    	if (minute < 10)	{
		    		lcd.write('0');
		    		lcd.print(minute,DEC);
		    	}
		    	else {
		    		lcd.print(minute,DEC);
		    	}
		    }
		    if (buttonPressed==KEYPAD_DOWN) {
		    	minute--;
				lcd.setCursor(8,1);
		    	if (minute > 59 && minute <= 255)
					minute = 59;
		    	if (minute < 10)	{
		    		lcd.write('0');
		    		lcd.print(minute,DEC);
		    	}
		    	else {
		    		lcd.print(minute,DEC);
		    	}
		    }
		    if (buttonPressed==KEYPAD_SELECT) {
				lcd.setCursor(0,0);
				lcd.print("   minutes OK   ");
				delay(2000);
		    }
		}	

	// verification: le nouvel horaire est-il semblable à un autre précédemment saisi?
	boolean doublon = false;
	for (byte i = 0; i < DISTRIB_MAX; i = i + 2) {
		if ( distribution[i] == heure && distribution[i + 1] == minute)
			doublon = true;
	}
	// si pas semblable, on l'enregistre
	if (doublon == false) {
		distribution[param1 * 2 - 2] = heure;
		distribution[param1 * 2 - 1] = minute;
		// et on classe par ordre croissant, heures puis minutes, en comparant chaque valeur avec la suivante (tri par selection)
		for (byte i = 0; i < DISTRIB_MAX; i = i + 2) {
			byte heure_mini = distribution[i];
			byte minute_mini = distribution[i + 1];
			for (byte j = i + 2; j < DISTRIB_MAX; j = j + 2) {
				// si l'heure est > que la suivante on fait le tri sans tenir compte des minutes 
				if ( distribution[j] < heure_mini ) {
					heure_mini = distribution[j];
					minute_mini = distribution[j + 1];
					distribution[j] = distribution[i];
					distribution[j + 1] = distribution[i + 1];
					distribution[i] = heure_mini;
					distribution[i + 1] = minute_mini;
				}
				// si l'heure est = à la suivante, alors on compare les minutes et on trie
				if ( distribution[j] == heure_mini ) {     
					if (distribution[j + 1] < minute_mini) {
						heure_mini = distribution[j];
						minute_mini = distribution[j + 1];
						distribution[j] = distribution[i];
						distribution[j + 1] = distribution[i + 1];
						distribution[i] = heure_mini;
						distribution[i + 1] = minute_mini;
					}
				}
			}
		}
		lcd.clear();
		lcd.setCursor(2,0);
		lcd.print("Horaire N");
		lcd.print((char)223);
		lcd.write(' ');
		lcd.print(param1, DEC);
		lcd.setCursor(5,1);
		lcd.print("ajout");
		lcd.write(byte(2));
		delay(2000);
		return true;
	}
	// si semblable, on passe
	else {
		lcd.clear();
		lcd.setCursor(2,0);
		lcd.print("Doublon avec");
		lcd.setCursor(0,1);
		lcd.print("horaire existant");
		delay(2000);
    return false;
	}
}

//Fonction pour charger les horaires de distrib de la mémoire eeprom vers le tableau "distribution"
void chargeDistrib()
{
	// Lecture et stockage des distrib dans le tableau distribution (inclus les valeurs 255)
	for (byte i = 0; i < DISTRIB_MAX; i++) {
		distribution [i] = EEPROM.read(i);
	}
}

//Fonction qui affiche les horaires programmés
void  afficheHoraires()
{
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Horaires de");
  lcd.setCursor(2,1);
  lcd.print("distribution");
  delay(2000);
  //lcd.autoscroll();
  for (byte i = 0; i < DISTRIB_MAX; i = i + 2) {
    byte Hour = distribution[i];
    byte Minute = distribution[i + 1];
    if (Hour != 255) {
    	lcd.clear();
  		lcd.home();
      if (Hour < 10)
		lcd.write('0');
      lcd.print(Hour);
      lcd.print(":");
      if (Minute < 10)
		lcd.write('0');
      lcd.print(Minute);
      lcd.print(" ");
      delay(1000);
     }
  }
  delay (1000);
  //lcd.noAutoscroll();
}

//Fonction qui ouvre et referme le clapet
void activeClapet() 
{
	servo_clapet.attach(Servo_clapetPin);
	servo_clapet.write(Servo_ouvert);
	delay(2000);
	servo_clapet.write(Servo_ferme);
	delay(1000);
	servo_clapet.detach();
	Serial.println();
}

//Fonction qui affiche un message au demarrage
void affiche_demarrage()
{
	lcd.clear();
	lcd.setCursor(0,1);
	lcd.print("v2.1");
	lcd.home();
	lcd.print("D");
	lcd.write(byte(2));
	lcd.print("marrage");
	for (byte i = 0; i < 8; i++) {
		lcd.write('.');
		delay(200);
	}
}


void setup() 
{
	 // partie software
	 Wire.begin();
	 DS3231_init(DS3231_CONTROL_INTCN);
	 lcd.createChar(0, haut);
	 lcd.createChar(1, bas);
	 lcd.createChar(2, e_accent_aigu);
	 lcd.begin(16, 2);
	 lcd.backlight();
	 chargeDistrib();
	 Serial.begin(57600);

	 // partie hardware
	 pinMode(LaserPin,OUTPUT);
	 pinMode (Servo_helicePin,OUTPUT);
	 pinMode (Servo_clapetPin,OUTPUT);
	 servo_clapet.attach(Servo_clapetPin);
	 servo_clapet.write(Servo_ferme);
	 delay(500);
	 servo_clapet.detach();
	 
	 affiche_demarrage();
}


void loop()
{
	static int8_t lastSecond = -1;
	DS3231_get(&t);
	if (t.sec != lastSecond) {
		printTime();
		for (byte i = 0; i < DISTRIB_MAX; i = i + 2) {
		  byte thisHour = distribution[i];
		  byte thisMinute = distribution[i + 1];
      // si c'est l'heure de distribution (heure et minute)
		  if (thisHour == t.hour && thisMinute == t.min && t.sec == 0) {
			if (ecranAllume == false)
				lcd.backlight();
			ecranAllume = true;
			changement_etat = true;
			byte pointille = 0;
			lcd.clear();
			lcd.setCursor(2,0);
			lcd.print("Distribution");
			lcd.setCursor(0,1);
			//on allume le laser
			digitalWrite(LaserPin, HIGH);
			//on active le servo helice
			servo_helice.attach(Servo_helicePin);
			//on fait tourner le moteur de l'helice pendant 1/4 de seconde
			servo_helice.write(Vitesse_helice);
			delay(250);
			servo_helice.detach();

			while (GamelleRemplie == false) {
			  //on initialise le recepteur de lumière
			  int RecepteurValue = analogRead(PhotorecepteurPin);
					  
			  // on verifie si le bol est plein 4 fois de suite
			  if (RecepteurValue > 100) {                  //si le laser detecte un obstacle
				delay(500);
				TourPrecedent++;
  				if (TourPrecedent == TourDeclenchant) {    //si c'est la 4eme fois consecutive, 
  				  //on en deduit que le bol est plein
  				  digitalWrite(LaserPin, LOW);
  				  //on ouvre le clapet
  				  activeClapet();
  				  TourPrecedent = 0;
  				  GamelleRemplie = true; 
  				}
			  }
			  else {
  				TourPrecedent = 0;
  				//faire tourner le moteur helice pendant 1/4 de seconde
  				servo_helice.attach(Servo_helicePin);
  				servo_helice.write(Vitesse_helice);
  				delay(250);
  				servo_helice.detach();
  				pointille ++;
				if ( pointille <= 64 && pointille % 4 == 0)
  					lcd.print('.'); // avance d'un pointillé toutes les secondes
  				//Si le moteur helice tourne à vide plus de 45 secondes, on affiche un message d'erreur et on sort de la boucle
  				DS3231_get(&t);
  				if ( t.sec == 45) {
  				  activeClapet();
				  lcd.clear();
				  lcd.setCursor(0,0);
				  lcd.print("  Distribution  ");
				  lcd.setCursor(0,1);
				  lcd.print("    partielle   ");				  
				  delay(2000);
				  lcd.clear();
				  lcd.setCursor(0,0);
				  lcd.print("  Ajoutez  des  ");
				  lcd.setCursor(0,1);
				  lcd.print("   croquettes   ");				  
				  delay(2000);
				  digitalWrite(LaserPin, LOW);
  				  break;
  				}
			  }          
			}
			GamelleRemplie = false;
			TourPrecedent = 0;
		  } 
		}
		lastSecond = t.sec;
		//hooks de commandes
		switch (lcd.button()) {

		  case KEYPAD_SELECT:
		  	{
				if (ecranAllume == false)
					lcd.backlight();
				ecranAllume = true;
				changement_etat = true;
				//gauche
				lcd.clear();
				lcd.home();
				lcd.write('<');
				lcd.setCursor(0,1);
				lcd.print("Saisir horaires");
				delay(2000);
				//haut
				lcd.clear();
				lcd.setCursor(8,0);
				lcd.write(byte(0));
				lcd.setCursor(1,1);
				lcd.print("Voir horaires");
				delay(2000);
				//droite
				lcd.clear();
				lcd.setCursor(15,0);
				lcd.write('>');
				lcd.setCursor(0,1);
				lcd.print("R");
				lcd.write(byte(2));
				lcd.print("gler horologe");
				delay(2000);
				//bas
				lcd.clear();
				lcd.setCursor(2,0);
				lcd.print("RAZ horaires");
				lcd.setCursor(8,1);
				lcd.write(byte(1));
				delay(2000);
			}
  			break;
			
		  case KEYPAD_UP:
			if (ecranAllume == false)
				lcd.backlight();
			ecranAllume = true;
			afficheHoraires();
			changement_etat = true;
			break;

		  case KEYPAD_DOWN:
			if (ecranAllume == false)
				lcd.backlight();
			ecranAllume = true;
			razDistrib();
			changement_etat = true;
			break;

		  case KEYPAD_LEFT:
			if (ecranAllume == false)
				lcd.backlight();
			ecranAllume = true;
			changeDistrib();
			changement_etat = true;
			break;

		  case KEYPAD_RIGHT:
			if (ecranAllume == false)
				lcd.backlight();
			ecranAllume = true;
			setRTC();
			changement_etat = true;
			break;
			
		  default:
			if (changement_etat == true) {
				endTime = millis() + veille;
				changement_etat = false;
			}
			if (millis() >=  endTime && ecranAllume == true) {
				lcd.noBacklight();
				ecranAllume = false;
			}
			break;
		}
  }
}
