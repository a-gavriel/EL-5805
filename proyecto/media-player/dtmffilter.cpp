#include "dtmffilter.h"


/******************************* SOURCE LICENSE *********************************
Copyright (c) 2019 MicroModeler.

A non-exclusive, nontransferable, perpetual, royalty-free license is granted to the Licensee to
use the following Information for academic, non-profit, or government-sponsored research purposes.
Use of the following Information under this License is restricted to NON-COMMERCIAL PURPOSES ONLY.
Commercial use of the following Information requires a separately executed written license agreement.

This Information is distributed WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

******************************* END OF LICENSE *********************************/

// A commercial license for MicroModeler DSP can be obtained at http://www.micromodeler.com/launch.jsp

#include <stdlib.h> // For malloc/free
#include <string.h> // For memset


/*

float DtmfTone_coefficients[15] = {
	// Scaled for floating point
	0.004875733235549571, 0, -0.004875733235549571, 1.9498782600726758, -0.993859827301148,// b0, b1, b2, a1, a2
	0.03125, -0.0625, 0.03125, 1.954205368960189, -0.9969701654390408,// b0, b1, b2, a1, a2
	0.000244140625, 0.00048828125, 0.000244140625, 1.951508578460346, -0.9968802840019438// b0, b1, b2, a1, a2

};

*/

DtmfToneType *DtmfTone_create( void )
{
	DtmfToneType *result = (DtmfToneType *)malloc( sizeof( DtmfToneType ) );	// Allocate memory for the object
	DtmfTone_init( result );											// Initialize it
	return result;																// Return the result
}

void DtmfTone_destroy( DtmfToneType *pObject )
{
	free( pObject );
}

 void DtmfTone_init( DtmfToneType * pThis )
{
	DtmfTone_reset( pThis );

}

 void DtmfTone_reset( DtmfToneType * pThis )
{
	memset( &pThis->state, 0, sizeof( pThis->state ) ); // Reset state to 0
	pThis->output = 0;									// Reset output

}

 int DtmfTone_filterBlock( DtmfToneType * pThis, float * pInput, float * pOutput, unsigned int count, float DtmfTone_coefficients[15])
{
	DtmfTone_executionState executionState;          // The executionState structure holds call data, minimizing stack reads and writes
	if( ! count ) return 0;                         // If there are no input samples, return immediately
	executionState.pInput = pInput;                 // Pointers to the input and output buffers that each call to filterBiquad() will use
	executionState.pOutput = pOutput;               // - pInput and pOutput can be equal, allowing reuse of the same memory.
	executionState.count = count;                   // The number of samples to be processed
	executionState.pState = pThis->state;                   // Pointer to the biquad's internal state and coefficients.
	executionState.pCoefficients = DtmfTone_coefficients;    // Each call to filterBiquad() will advance pState and pCoefficients to the next biquad

	// The 1st call to DtmfTone_filterBiquad() reads from the caller supplied input buffer and writes to the output buffer.
	// The remaining calls to filterBiquad() recycle the same output buffer, so that multiple intermediate buffers are not required.

	DtmfTone_filterBiquad( &executionState );		// Run biquad #0
	executionState.pInput = executionState.pOutput;         // The remaining biquads will now re-use the same output buffer.

	DtmfTone_filterBiquad( &executionState );		// Run biquad #1

	DtmfTone_filterBiquad( &executionState );		// Run biquad #2

	// At this point, the caller-supplied output buffer will contain the filtered samples and the input buffer will contain the unmodified input samples.
	return count;		// Return the number of samples processed, the same as the number of input samples

}

 void DtmfTone_filterBiquad( DtmfTone_executionState * pExecState )
{
	// Read state variables
	float w0, x0;
	float w1 = pExecState->pState[0];
	float w2 = pExecState->pState[1];

	// Read coefficients into work registers
	float b0 = *(pExecState->pCoefficients++);
	float b1 = *(pExecState->pCoefficients++);
	float b2 = *(pExecState->pCoefficients++);
	float a1 = *(pExecState->pCoefficients++);
	float a2 = *(pExecState->pCoefficients++);

	// Read source and target pointers
	float *pInput  = pExecState->pInput;
	float *pOutput = pExecState->pOutput;
	short count = pExecState->count;
	float accumulator;

	// Loop for all samples in the input buffer
	while( count-- )
	{
		// Read input sample
		x0 = *(pInput++);

		// Run feedback part of filter
		accumulator  = w2 * a2;
		accumulator += w1 * a1;
		accumulator += x0 ;

		w0 = accumulator ;

		// Run feedforward part of filter
		accumulator  = w0 * b0;
		accumulator += w1 * b1;
		accumulator += w2 * b2;

		w2 = w1;		// Shuffle history buffer
		w1 = w0;

		// Write output
		*(pOutput++) = accumulator ;
	}

	// Write state variables
	*(pExecState->pState++) = w1;
	*(pExecState->pState++) = w2;

}
