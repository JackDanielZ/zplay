#define EFL_BETA_API_SUPPORT
#define EFL_EO_API_SUPPORT

#include <Elementary.h>
#include <Emotion.h>

typedef struct
{
   Eina_Stringshare *id;
   Eina_Stringshare *path;
} Song_Info;

static Eo *_emo = NULL;

static Eina_List *_queue = NULL, *_cur_queue_item = NULL;
static Eina_Bool _pause_on_notfound = EINA_FALSE;

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
   printf("STOPPED\n");
}

static void
_item_play(Song_Info *info)
{
   if (ecore_file_exists(info->path))
     {
        emotion_object_play_set(_emo, EINA_FALSE);
        emotion_object_position_set(_emo, 0.0);
        emotion_object_file_set(_emo, info->path);
        emotion_object_play_set(_emo, EINA_TRUE);
     }
   else printf("FILE_NOT_FOUND: %s\n", info->id);
}

static Song_Info *
_item_find(const char *id)
{
   Eina_List *itr;
   Song_Info *info;
   Eina_Stringshare *shr = eina_stringshare_add(id);
   EINA_LIST_FOREACH(_queue, itr, info)
     {
        if (info->id == shr) goto end;
     }
   info = NULL;
end:
   eina_stringshare_del(shr);
   return info;
}

static Eina_Bool
_on_stdin(void *data EINA_UNUSED, Ecore_Fd_Handler *fdh EINA_UNUSED)
{
   static Eina_Bool _progress = EINA_FALSE;
   char *line = alloca(1024);
   ssize_t nb = read(STDIN_FILENO, line, 1024);

   while (nb > 0)
     {
        char *eol = strchr(line, '\n');
        if (eol) *eol = '\0';
        int len = strlen(line);
        if (!strcmp(line, "PAUSE")) emotion_object_play_set(_emo, EINA_FALSE);
        else if (!strncmp(line, "PLAY", 4))
          {
             if (line[4] == ' ')
               {
                  Song_Info *info = _item_find(line + 5);
                  if (info)
                    {
                       _cur_queue_item = eina_list_data_find_list(_queue, info);
                       _item_play(info);
                    }
               }
             else emotion_object_play_set(_emo, EINA_TRUE);
          }
        else if (!strcmp(line, "STOP"))
          {
             emotion_object_play_set(_emo, EINA_FALSE);
             emotion_object_position_set(_emo, 0.0);
          }
        else if (!strcmp(line, "NEXT"))
          {
             Eina_List *next = eina_list_next(_cur_queue_item);
             if (!next) next = _queue;
             _cur_queue_item = next;
             _item_play(eina_list_data_get(next));
          }
        else if (!strcmp(line, "PREV"))
          {
             Eina_List *prev = eina_list_prev(_cur_queue_item);
             if (!prev) prev = eina_list_last(_queue);
             _cur_queue_item = prev;
             _item_play(eina_list_data_get(prev));
          }
        else if (!strcmp(line, "SHOW_PROGRESS"))
          {
             if (!_progress)
               {
                  efl_event_callback_add
                     (_emo, EFL_CANVAS_VIDEO_EVENT_POSITION_CHANGE, _media_position_update, NULL);
                  _progress = EINA_TRUE;
               }
          }
        else if (!strcmp(line, "HIDE_PROGRESS"))
          {
             efl_event_callback_del
                (_emo, EFL_CANVAS_VIDEO_EVENT_POSITION_CHANGE, _media_position_update, NULL);
             _progress = EINA_FALSE;
          }
        else if (!strncmp(line, "POSITION", 8))
          {
             if (line[8] == ' ')
               {
                  int pos = atoi(line + 9);
                  emotion_object_position_set(_emo, pos);
               }
             else
               {
                  int pos = (int)emotion_object_position_get(_emo);
                  int total = (int)emotion_object_play_length_get(_emo);
                  printf("POSITION: %d / %d\n", pos, total);
               }
          }
        else if (!strcmp(line, "PAUSE_ON_NOTFOUND"))
          {
             _pause_on_notfound = EINA_TRUE;
          }
        else if (!strncmp(line, "ADD_TO_QUEUE ", 13))
          {
             Song_Info *info;
             char *spc = strchr(line + 13, ' ');
             *spc = '\0';
             info = _item_find(line + 13);
             if (info)
               {
                  eina_stringshare_del(info->path);
                  info->path = eina_stringshare_add(spc + 1);
               }
             else
               {
                  info = calloc(1, sizeof(*info));
                  info->id = eina_stringshare_add(line + 13);
                  info->path = eina_stringshare_add(spc + 1);
                  _queue = eina_list_append(_queue, info);
               }
          }
        else if (!strncmp(line, "REMOVE_FROM_QUEUE ", 18))
          {
             Song_Info *info = _item_find(line + 18);
             if (info) _queue = eina_list_remove(_queue, info);
          }
        else if (!strncmp(line, "VOLUME", 6))
          {
             if (line[6] == ' ')
               {
                  int vol = atoi(line + 7);
                  emotion_object_audio_volume_set(_emo, vol / 100.0);
               }
             else
               {
                  double vol = emotion_object_audio_volume_get(_emo);
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

   _emo = emotion_object_add(e);
   emotion_object_init(_emo, NULL);
   efl_event_callback_add
      (_emo, EFL_CANVAS_VIDEO_EVENT_PLAYBACK_STOP, _media_finished, NULL);

   if (argc == 2)
     {
        emotion_object_file_set(_emo, argv[1]);
        emotion_object_play_set(_emo, EINA_TRUE);
     }
   ecore_main_fd_handler_add(STDIN_FILENO, ECORE_FD_READ, _on_stdin, _emo, NULL, NULL);

   elm_run();

   elm_shutdown();
   emotion_shutdown();
   eina_shutdown();
   return 0;
}
