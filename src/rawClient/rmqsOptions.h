/****************************************************************************
MIT License

Copyright (c) 2023 Gianfranco Giugliano

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
****************************************************************************/
//---------------------------------------------------------------------------
#ifndef rmqsOptionsH
#define rmqsOptionsH
//---------------------------------------------------------------------------
#define RMQS_PROTOCOL_FUNCTIONS_INLINE    0
#define RMQS_CLIENT_FUNCTIONS_INLINE      0
#define RMQS_BUFFERING_FUNCTIONS_INLINE   0
#define RMQS_MEMORY_FUNCTIONS_INLINE      0
//---------------------------------------------------------------------------
#define rmqsInLine extern inline
//---------------------------------------------------------------------------
#if RMQS_PROTOCOL_FUNCTIONS_INLINE
#define rmqsProtoFunc rmqsInLine
#else
#define rmqsProtoFunc 
#endif
//---------------------------------------------------------------------------
#if RMQS_CLIENT_FUNCTIONS_INLINE
#define rmqsClientFunc rmqsInLine
#else
#define rmqsClientFunc
#endif
//---------------------------------------------------------------------------
#if RMQS_BUFFERING_FUNCTIONS_INLINE
#define rmqsBufferingFunc rmqsInLine
#else
#define rmqsBufferingFunc
#endif
//---------------------------------------------------------------------------
#if RMQS_MEMORY_FUNCTIONS_INLINE
#define rmqsMemoryFunc rmqsInLine
#else
#define rmqsMemoryFunc
#endif
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
