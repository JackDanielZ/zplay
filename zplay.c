#define EFL_BETA_API_SUPPORT
#define EFL_EO_API_SUPPORT

#include <Elementary.h>
#include <Emotion.h>

static void
_media_position_update(void *data EINA_UNUSED, const Efl_Event *ev)
{
   int pos = (int)emotion_object_position_get(ev->object);
   int total = (int)emotion_object_play_length_get(ev->object);
   printf("POSITION: %d / %d\n", pos, total);
}

static void
_media_finished(void *data EINA_UNUSED, const Efl_Event *ev)
{
   emotion_object_play_set(ev->object, EINA_FALSE);
   emotion_object_position_set(ev->object, 0.0);
}

static Eina_Bool
_on_stdin(void *data, Ecore_Fd_Handler *fdh EINA_UNUSED)
{
   static Eina_Bool _progress = EINA_FALSE;
   Eo *emo = data;
   char *line = alloca(1024);
   ssize_t nb = read(STDIN_FILENO, line, 1024);

   while (nb > 0)
     {
        char *eol = strchr(line, '\n');
        if (eol) *eol = '\0';
        int len = strlen(line);
        if (!strcmp(line, "PAUSE")) emotion_object_play_set(emo, EINA_FALSE);
        else if (!strcmp(line, "PLAY")) emotion_object_play_set(emo, EINA_TRUE);
        else if (!strcmp(line, "STOP"))
          {
             emotion_object_play_set(emo, EINA_FALSE);
             emotion_object_position_set(emo, 0.0);
          }
        else if (!strcmp(line, "SHOW_PROGRESS"))
          {
             if (!_progress)
               {
                  efl_event_callback_add
                     (emo, EFL_CANVAS_VIDEO_EVENT_POSITION_CHANGE, _media_position_update, NULL);
                  _progress = EINA_TRUE;
               }
          }
        else if (!strcmp(line, "HIDE_PROGRESS"))
          {
             efl_event_callback_del
                (emo, EFL_CANVAS_VIDEO_EVENT_POSITION_CHANGE, _media_position_update, NULL);
             _progress = EINA_FALSE;
          }
        else if (!strncmp(line, "POSITION", 8))
          {
             if (line[8] == ' ')
               {
                  int pos = atoi(line + 9);
                  emotion_object_position_set(emo, pos);
               }
             else
               {
                  int pos = (int)emotion_object_position_get(emo);
                  int total = (int)emotion_object_play_length_get(emo);
                  printf("POSITION: %d / %d\n", pos, total);
               }
          }
        else if (!strncmp(line, "FILE ", 5))
          {
             emotion_object_file_set(emo, line + 5);
          }
        else if (!strncmp(line, "VOLUME", 6))
          {
             if (line[6] == ' ')
               {
                  int vol = atoi(line + 7);
                  emotion_object_audio_volume_set(emo, vol / 100.0);
               }
             else
               {
                  double vol = emotion_object_audio_volume_get(emo);
                  printf("VOLUME: %d\n", (int)(vol * 100));
               }
          }
        line += len + 1;
        nb -= (len + 1);
        if (nb < 0) nb = 0;
     }
   if (nb < 0)
     {
        fprintf(stderr, "ERROR: could not read from stdin: %s\n", strerror(errno));
        elm_exit();
        return ECORE_CALLBACK_CANCEL;
     }
   return ECORE_CALLBACK_RENEW;
}

int main(int argc, char **argv)
{
   eina_init();
   emotion_init();
   elm_init(argc, argv);

   Eo *e = evas_new();
   evas_output_method_set(e, evas_render_method_lookup("buffer"));

   Eo *emo = emotion_object_add(e);
   emotion_object_init(emo, NULL);
   efl_event_callback_add
      (emo, EFL_CANVAS_VIDEO_EVENT_PLAYBACK_STOP, _media_finished, NULL);

   if (argc == 2)
     {
        emotion_object_file_set(emo, argv[1]);
        emotion_object_play_set(emo, EINA_TRUE);
     }
   ecore_main_fd_handler_add(STDIN_FILENO, ECORE_FD_READ, _on_stdin, emo, NULL, NULL);

   elm_run();

   elm_shutdown();
   emotion_shutdown();
   eina_shutdown();
   return 0;
}
