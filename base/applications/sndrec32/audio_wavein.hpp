#ifndef _AUDIOWAVEIN_H_
#define _AUDIOWAVEIN_H_



#include "audio_def.hpp"
#include "audio_format.hpp"
#include "audio_receiver.hpp"



_AUDIO_NAMESPACE_START_




enum audio_wavein_status { WAVEIN_NOTREADY, WAVEIN_READY, 
						   WAVEIN_RECORDING, WAVEIN_ERR,
						   WAVEIN_STOP, WAVEIN_FLUSHING
						
						  };





class audio_wavein
{
	private:



		//
		// The new recording thread sends message to this procedure
		// about open recording, close, and sound data recorded
		//

		static DWORD WINAPI recording_procedure( LPVOID );

		//
		// When this event is signaled, then the previsiously created
		// recording thread will wake up and start recording audio
		// and will pass audio data to an `audio_receiver' object.
		//

		HANDLE wakeup_recthread;
		HANDLE data_flushed_event;




	protected:


//TODO: puts these structs in private?!




		//
		// Audio wavein device stuff
		//

		WAVEFORMATEX   wave_format;
		WAVEHDR        * wave_headers;
		HWAVEIN        wavein_handle;



	

		audio_format aud_info;
		
		audio_receiver & audio_rcvd;


		
		//
		// Audio Recorder Thread id
		//

		DWORD     recthread_id;


		

		//
		// Object status
		//

		audio_wavein_status status;







		//
		// How many seconds of audio
		// can record the internal buffer
		// before flushing audio data 
		// to the `audio_receiver' class?
		//

		float buf_secs;


		//
		// The temporary buffers for the audio
		// data incoming from the wavein device
		// and its size, and its total number.
		//

		BYTE * main_buffer;
		unsigned int mb_size;

		unsigned int buffers;





		//
		// Protected Functions
		//


		//initialize all structures and variables.
		void init_( void );

		void alloc_buffers_mem_( unsigned int, float );
		void free_buffers_mem_( void );

		void init_headers_( void );
		void prep_headers_( void );
		void unprep_headers_( void );
		void add_buffers_to_driver_( void );







	public:


		//
		// Ctors
		//

		audio_wavein(
			const audio_format & a_info, audio_receiver & a_receiver )

			: wave_headers( 0 ),
			aud_info( a_info ), audio_rcvd( a_receiver ), 
			status( WAVEIN_NOTREADY ), main_buffer( 0 ), mb_size( 0 ),
			buffers( _AUDIO_DEFAULT_WAVEINBUFFERS )
		{

			//
			// Initializing internal wavein data
			//
			
			
			init_();

			aud_info = a_info;
		}







		//
		// Dtor
		//

		~audio_wavein( void )
		{
			
			//close(); TODO!

		}



		//
		// Public functions
		//

		void open( void );
		void close ( void );


		void start_recording( void );
		void stop_recording( void );



		audio_wavein_status current_status ( void ) const
		{
			return status;
		}

		float buffer_secs( void ) const
		{ return buf_secs; }


		void buffer_secs( float bsecs )
		{ 
			//
			// Some checking
			//

			if ( bsecs <= 0 )
				return;


			//
			// Set seconds lenght for each
			// buffer.
			//

			buf_secs = bsecs; 
		}


		unsigned int total_buffers( void ) const
		{ return buffers; }



		void total_buffers( unsigned int tot_bufs )
		{

			//
			// Some checking
			//

			if ( tot_bufs == 0 )
				return;

			
			//
			// Sets the number of total buffers.
			//

			buffers = tot_bufs;
		}


		audio_format format( void ) const
		{ return aud_info; }


	
};




_AUDIO_NAMESPACE_END_




#endif //ifdef _AUDIOWAVEIN_H_
