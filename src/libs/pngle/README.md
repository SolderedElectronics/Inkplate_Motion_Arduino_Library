# pngle library
Original source: https://github.com/kikuchan/pngle

## Note
This library is modified by Soldered Electronics. Modifications added:

- Added void ```*sessionHandler;``` in ```pngle_t```typedef
- Added setter ```void pngle_set_session_handle(pngle_t *pngle, void *sessionHandlePtr);```
- Added getter ```void* pngle_get_session_handle(pngle_t *pngle);```

Note that these modifications **must be** transfered while updating the pngle library!