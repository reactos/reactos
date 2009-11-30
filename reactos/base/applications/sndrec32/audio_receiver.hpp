#ifndef _AUDIORECEIVER_DEF__H_
#define _AUDIORECEIVER_DEF__H_



#include "audio_def.hpp"




_AUDIO_NAMESPACE_START_




//TODO: differenziare audio_receiver da audio_sink?
//TODO: creare un audio_receiver Tee





class audio_receiver
{

		//
		// The `audio_wavein' class, while is
		// recording audio, has to access to 
		// protected members of `audio_receiver'
		// such as `bytes_received' protected
		// variable.
		//

		friend class audio_wavein;






	protected:


		unsigned int bytes_received;
		unsigned int status;



	public:
		
		
		//
		// Ctors
		//

		audio_receiver( void )
			: bytes_received( 0 )
		{  }





		//
		// Dtor
		//

		virtual ~audio_receiver( void )
		{  }



		
		//
		// Public Functions
		//

		virtual void audio_receive( unsigned char *, unsigned int ) = 0;
		
		//virtual void start_rec( void ) = 0;
		//virtual void stop_rec( void ) = 0;



		void set_b_received( unsigned int r )
		{ bytes_received = r; }
};







_AUDIO_NAMESPACE_END_







#endif //ifdef _AUDIORECEIVER_DEF__H_
