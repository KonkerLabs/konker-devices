//for LED status
#include <Ticker.h>
Ticker _bticker;

//initial values to identify the ESP by blinking
int  _bblinksTimes=1;
int  _bledWaitOffPeriod=4;

//auxiliar variables to blink id
int  _bledBlinksCount=0;
int  _bledOffState=1;
int  _bledWaitCount=0;
int _STATUS_LED=2;


void  _btick()
{
  //toggle state
  int state = digitalRead(_STATUS_LED);  // get the current state of GPIO1 pin
  digitalWrite(_STATUS_LED, !state);     // set pin to the opposite state
  if(_bledOffState==1){
    _bledBlinksCount=_bledBlinksCount+state;
  }else{
    _bledBlinksCount=_bledBlinksCount+!state;
  }

}


//blink  function
void  _bblink()
{
  if( _bledBlinksCount<_bblinksTimes){
    //tick and count +1 blink
     _btick();

  }else{
    //set led off
    digitalWrite(_STATUS_LED, _bledOffState);

    if( _bledWaitCount< _bledWaitOffPeriod){
      // +1 count to keep led off
       _bledWaitCount++;
    }else{
      //reached ledWaitOffPeriod period.
      // Reset variables start all over ain

      //set led off
      digitalWrite(_STATUS_LED,_bledOffState);

      _bledWaitCount=0;
      _bledBlinksCount=0;
    }
  }
}


//startBlinkID  function
//set ID zero to stop
void startBlinkID(int ID)
{
    if(ID>0){
      _bblinksTimes=ID;
      _bticker.attach(0.3,  _bblink);
    }else{
      _bticker.detach();
    }

}
