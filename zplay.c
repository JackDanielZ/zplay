#define EFL_BETA_API_SUPPORT
#define EFL_EO_API_SUPPORT

#include <Elementary.h>
#include <Emotion.h>

static Eina_Bool _piped = EINA_FALSE;

static void
_media_position_update(void *data EINA_UNUSED, const Efl_Event *ev)
{
   int pos = (int)emotion_object_position_get(ev->object);
   int len = (int)emotion_object_play_length_get(ev->object);
   printf("Position: %d / %d\n", pos, len);
}

static void
_media_finished(void *data EINA_UNUSED, const Efl_Event *ev)
{
   emotion_object_play_set(ev->object, EINA_FALSE);
   if (!_piped) elm_exit();
}

static Eina_Bool
_on_stdin(void *data, Ecore_Fd_Handler *fdh EINA_UNUSED)
{
   static Eina_Bool _progress = EINA_FALSE;
   Eo *emo = data;
   char *line = NULL;
   size_t len = 0;
   ssize_t r = getline(&line, &len, stdin);

   if (r < 0)
     {
        fprintf(stderr, "ERROR: could not read from stdin: %s\n", strerror(errno));
        elm_exit();
        return ECORE_CALLBACK_CANCEL;
     }
   if (!strcmp(line, "PAUSE\n")) emotion_object_play_set(emo, EINA_FALSE);
   else if (!strcmp(line, "PLAY\n")) emotion_object_play_set(emo, EINA_TRUE);
   else if (!strcmp(line, "STOP\n"))
     {
        emotion_object_play_set(emo, EINA_FALSE);
        emotion_object_position_set(emo, 0.0);
     }
   else if (!strcmp(line, "SHOW_PROGRESS\n"))
     {
        if (!_progress)
          {
             efl_event_callback_add
                (emo, EFL_CANVAS_VIDEO_EVENT_POSITION_CHANGE, _media_position_update, NULL);
             _progress = EINA_TRUE;
          }
     }
   else if (!strcmp(line, "HIDE_PROGRESS\n"))
     {
        efl_event_callback_del
           (emo, EFL_CANVAS_VIDEO_EVENT_POSITION_CHANGE, _media_position_update, NULL);
        _progress = EINA_FALSE;
     }
   else if (!strncmp(line, "FILE ", 5))
     {
        char *eol = strchr(line, '\n');
        *eol = '\0';
        emotion_object_file_set(emo, line + 5);
     }
   free(line);
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
   _piped = !isatty(fileno(stdin));
   if (!_piped)
     {
        if (argc != 2)
          {
             fprintf(stderr, "Argument (file to play) required\n");
             exit(1);
          }
     }
   else
     {
        ecore_main_fd_handler_add(STDIN_FILENO, ECORE_FD_READ, _on_stdin, emo, NULL, NULL);
     }

   elm_run();

   elm_shutdown();
   emotion_shutdown();
   eina_shutdown();
   return 0;
}
