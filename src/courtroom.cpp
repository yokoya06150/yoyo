#include "courtroom.h"

Courtroom::Courtroom(AOApplication *p_ao_app) : QMainWindow()
{
  ao_app = p_ao_app;
  #ifdef BASSAUDIO
  // Change the default audio output device to be the one the user has given
  // in his config.ini file for now.
  unsigned int a = 0;
  BASS_DEVICEINFO info;

  if (ao_app->get_audio_output_device() == "default")
  {
      BASS_Init(-1, 48000, BASS_DEVICE_LATENCY, nullptr, nullptr);
      load_bass_opus_plugin();
  }
  else
  {
      for (a = 0; BASS_GetDeviceInfo(a, &info); a++)
      {
          if (ao_app->get_audio_output_device() == info.name)
          {
              BASS_SetDevice(a);
              BASS_Init(static_cast<int>(a), 48000, BASS_DEVICE_LATENCY, nullptr, nullptr);
              load_bass_opus_plugin();
              qDebug() << info.name << "was set as the default audio output device.";
              break;
          }
      }
  }
  #elif defined QTAUDIO

  if (ao_app->get_audio_output_device() != "default")
  {
      foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
          {
          if (ao_app->get_audio_output_device() == deviceInfo.deviceName())
          {
              ao_app->QtAudioDevice = deviceInfo;
              qDebug() << deviceInfo.deviceName() << "was set as the default audio output device.";
              break;
          }
      }
  }
  #endif

  keepalive_timer = new QTimer(this);
  keepalive_timer->start(60000);

  chat_tick_timer = new QTimer(this);

  text_delay_timer = new QTimer(this);
  text_delay_timer->setSingleShot(true);

  sfx_delay_timer = new QTimer(this);
  sfx_delay_timer->setSingleShot(true);

  music_player = new AOMusicPlayer(this, ao_app);
  music_player->set_volume(0);

  sfx_player = new AOSfxPlayer(this, ao_app);
  sfx_player->set_volume(0);

  objection_player = new AOSfxPlayer(this, ao_app);
  objection_player->set_volume(0);

  blip_player = new AOBlipPlayer(this, ao_app);
  blip_player->set_volume(0);

  modcall_player = new AOSfxPlayer(this, ao_app);
  modcall_player->set_volume(50);

  ui_background = new AOImage(this, ao_app);

  ui_viewport = new QWidget(this);
  ui_vp_background = new AOScene(ui_viewport, ao_app);
  ui_vp_speedlines = new AOMovie(ui_viewport, ao_app);
  ui_vp_speedlines->set_play_once(false);
  ui_vp_player_char = new AOCharMovie(ui_viewport, ao_app);
  ui_vp_sideplayer_char = new AOCharMovie(ui_viewport, ao_app);
  ui_vp_sideplayer_char->hide();
  ui_vp_desk = new AOScene(ui_viewport, ao_app);
  ui_vp_legacy_desk = new AOScene(ui_viewport, ao_app);

  ui_vp_evidence_display = new AOEvidenceDisplay(this, ao_app);

  ui_vp_chatbox = new AOImage(this, ao_app);
  ui_vp_showname = new QLabel(ui_vp_chatbox);
  ui_vp_showname->setAlignment(Qt::AlignHCenter);
  ui_vp_chat_arrow = new AOMovie(ui_vp_chatbox, ao_app);
  ui_vp_chat_arrow->set_play_once(false);

  ui_vp_message = new QTextEdit(this);
  ui_vp_message->setFrameStyle(QFrame::NoFrame);
  ui_vp_message->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ui_vp_message->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ui_vp_message->setReadOnly(true);

  ui_vp_testimony = new AOMovie(this, ao_app);
  ui_vp_testimony->set_play_once(false);
  ui_vp_testimony->setAttribute(Qt::WA_TransparentForMouseEvents);
  ui_vp_effect = new AOMovie(this, ao_app);
  ui_vp_effect->setAttribute(Qt::WA_TransparentForMouseEvents);
  ui_vp_wtce = new AOMovie(this, ao_app);
  ui_vp_wtce->setAttribute(Qt::WA_TransparentForMouseEvents);
  ui_vp_objection = new AOMovie(this, ao_app);
  ui_vp_objection->setAttribute(Qt::WA_TransparentForMouseEvents);

  ui_ic_chatlog = new QTextEdit(this);
  ui_ic_chatlog->setReadOnly(true);

  log_maximum_blocks = ao_app->get_max_log_size();
  log_goes_downwards = ao_app->get_log_goes_downwards();

  ui_ms_chatlog = new AOTextArea(this);
  ui_ms_chatlog->setReadOnly(true);
  ui_ms_chatlog->setOpenExternalLinks(true);
  ui_ms_chatlog->hide();

  ui_server_chatlog = new AOTextArea(this);
  ui_server_chatlog->setReadOnly(true);
  ui_server_chatlog->setOpenExternalLinks(true);

  ui_area_list = new QListWidget(this);
  ui_area_list->hide();
  ui_music_list = new QTreeWidget(this);
  ui_music_list->setColumnCount(1);
  ui_music_list->setHeaderHidden(true);
  ui_music_list->header()->setStretchLastSection(false);
  ui_music_list->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

  ui_music_display = new AOMovie(this, ao_app);
  ui_music_display->set_play_once(false);
  ui_music_display->setAttribute(Qt::WA_TransparentForMouseEvents);

  ui_music_name = new ScrollText(ui_music_display);
  ui_music_name->setText(tr("None"));
  ui_music_name->setAttribute(Qt::WA_TransparentForMouseEvents);

  ui_ic_chat_name = new QLineEdit(this);
  ui_ic_chat_name->setFrame(false);
  ui_ic_chat_name->setPlaceholderText(tr("Showname"));

  ui_ic_chat_message = new AOLineEdit(this);
  ui_ic_chat_message->setFrame(false);
  ui_ic_chat_message->setPlaceholderText(tr("Message"));
  ui_ic_chat_message->preserve_selection(true);
  //todo: filter out \n from showing up as that commonly breaks the chatlog and can be spammed to hell

  ui_muted = new AOImage(ui_ic_chat_message, ao_app);
  ui_muted->hide();

  ui_ooc_chat_message = new QLineEdit(this);
  ui_ooc_chat_message->setFrame(false);

  ui_ooc_chat_name = new QLineEdit(this);
  ui_ooc_chat_name->setFrame(false);
  ui_ooc_chat_name->setPlaceholderText(tr("Name"));
  ui_ooc_chat_name->setMaxLength(30);
  ui_ooc_chat_name->setText(p_ao_app->get_default_username());

  //ui_area_password = new QLineEdit(this);
  //ui_area_password->setFrame(false);
  ui_music_search = new QLineEdit(this);
  ui_music_search->setFrame(false);
  ui_music_search->setPlaceholderText(tr("Search"));

  initialize_emotes();

  ui_pos_dropdown = new QComboBox(this);
  ui_pos_dropdown->addItem("wit");
  ui_pos_dropdown->addItem("def");
  ui_pos_dropdown->addItem("pro");
  ui_pos_dropdown->addItem("jud");
  ui_pos_dropdown->addItem("hld");
  ui_pos_dropdown->addItem("hlp");
  ui_pos_dropdown->addItem("jur");
  ui_pos_dropdown->addItem("sea");

  ui_iniswap_dropdown = new QComboBox(this);
  ui_iniswap_remove = new AOButton(this, ao_app);

  ui_sfx_dropdown = new QComboBox(this);
  ui_sfx_remove = new AOButton(this, ao_app);

  ui_effects_dropdown = new QComboBox(this);

  ui_defense_bar = new AOImage(this, ao_app);
  ui_prosecution_bar = new  AOImage(this, ao_app);

  ui_music_label = new QLabel(this);
  ui_sfx_label = new QLabel(this);
  ui_blip_label = new QLabel(this);

  ui_hold_it = new AOButton(this, ao_app);
  ui_objection = new AOButton(this, ao_app);
  ui_take_that = new AOButton(this, ao_app);

  ui_ooc_toggle = new AOButton(this, ao_app);
  ui_witness_testimony = new AOButton(this, ao_app);
  ui_cross_examination = new AOButton(this, ao_app);
  ui_guilty = new AOButton(this, ao_app);
  ui_not_guilty = new AOButton(this, ao_app);

  ui_change_character = new AOButton(this, ao_app);
  ui_reload_theme = new AOButton(this, ao_app);
  ui_call_mod = new AOButton(this, ao_app);
  ui_settings = new AOButton(this, ao_app);
  ui_announce_casing = new AOButton(this, ao_app);
  ui_switch_area_music = new AOButton(this, ao_app);

  ui_pre = new QCheckBox(this);
  ui_pre->setText(tr("Pre"));

  ui_flip = new QCheckBox(this);
  ui_flip->setText(tr("Flip"));
  ui_flip->hide();

  ui_guard = new QCheckBox(this);
  ui_guard->setText(tr("Guard"));
  ui_guard->hide();

  ui_additive = new QCheckBox(this);
  ui_additive->setText(tr("Additive"));
  ui_additive->hide();

  ui_casing = new QCheckBox(this);
  ui_casing->setChecked(ao_app->get_casing_enabled());
  ui_casing->setText(tr("Casing"));
  ui_casing->hide();

  ui_showname_enable = new QCheckBox(this);
  ui_showname_enable->setChecked(ao_app->get_showname_enabled_by_default());
  ui_showname_enable->setText(tr("Shownames"));

  ui_pre_non_interrupt = new QCheckBox(this);
  ui_pre_non_interrupt->setText(tr("No Interrupt"));
  ui_pre_non_interrupt->hide();

  ui_custom_objection = new AOButton(this, ao_app);
  ui_realization = new AOButton(this, ao_app);
  ui_screenshake = new AOButton(this, ao_app);
  ui_mute = new AOButton(this, ao_app);

  ui_defense_plus = new AOButton(this, ao_app);
  ui_defense_minus = new AOButton(this, ao_app);

  ui_prosecution_plus = new AOButton(this, ao_app);
  ui_prosecution_minus = new AOButton(this, ao_app);

  ui_text_color = new QComboBox(this);

  ui_music_slider = new QSlider(Qt::Horizontal, this);
  ui_music_slider->setRange(0, 100);
  ui_music_slider->setValue(ao_app->get_default_music());

  ui_sfx_slider = new QSlider(Qt::Horizontal, this);
  ui_sfx_slider->setRange(0, 100);
  ui_sfx_slider->setValue(ao_app->get_default_sfx());

  ui_blip_slider = new QSlider(Qt::Horizontal, this);
  ui_blip_slider->setRange(0, 100);
  ui_blip_slider->setValue(ao_app->get_default_blip());

  ui_mute_list = new QListWidget(this);

  ui_pair_list = new QListWidget(this);
  ui_pair_offset_spinbox = new QSpinBox(this);
  ui_pair_offset_spinbox->setRange(-100,100);
  ui_pair_offset_spinbox->setSuffix(tr("% offset"));
  ui_pair_button = new AOButton(this, ao_app);

  ui_evidence_button = new AOButton(this, ao_app);

  initialize_evidence();

  construct_char_select();

  connect(keepalive_timer, SIGNAL(timeout()), this, SLOT(ping_server()));

  connect(ui_vp_objection, SIGNAL(done()), this, SLOT(objection_done()));
  connect(ui_vp_player_char, SIGNAL(done()), this, SLOT(preanim_done()));
  connect(ui_vp_player_char, SIGNAL(shake()), this, SLOT(do_screenshake()));
  connect(ui_vp_player_char, SIGNAL(flash()), this, SLOT(do_flash()));
  connect(ui_vp_player_char, SIGNAL(play_sfx(QString)), this, SLOT(play_char_sfx(QString)));

  connect(text_delay_timer, SIGNAL(timeout()), this, SLOT(start_chat_ticking()));
  connect(sfx_delay_timer, SIGNAL(timeout()), this, SLOT(play_sfx()));

  connect(chat_tick_timer, SIGNAL(timeout()), this, SLOT(chat_tick()));

  connect(ui_pos_dropdown, SIGNAL(currentIndexChanged(int)), this, SLOT(on_pos_dropdown_changed(int)));

  connect(ui_iniswap_dropdown, SIGNAL(activated(int)), this, SLOT(on_iniswap_dropdown_changed(int)));
  connect(ui_iniswap_remove, SIGNAL(clicked()), this, SLOT(on_iniswap_remove_clicked()));

  connect(ui_sfx_dropdown, SIGNAL(activated(int)), this, SLOT(on_sfx_dropdown_changed(int)));
  connect(ui_sfx_remove, SIGNAL(clicked()), this, SLOT(on_sfx_remove_clicked()));

  connect(ui_effects_dropdown, SIGNAL(activated(int)), this, SLOT(on_effects_dropdown_changed(int)));

  connect(ui_mute_list, SIGNAL(clicked(QModelIndex)), this, SLOT(on_mute_list_clicked(QModelIndex)));

  connect(ui_ic_chat_message, SIGNAL(returnPressed()), this, SLOT(on_chat_return_pressed()));

  connect(ui_ooc_chat_message, SIGNAL(returnPressed()), this, SLOT(on_ooc_return_pressed()));

  connect(ui_music_list, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(on_music_list_double_clicked(QTreeWidgetItem*, int)));
  connect(ui_area_list, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(on_area_list_double_clicked(QModelIndex)));

  connect(ui_hold_it, SIGNAL(clicked()), this, SLOT(on_hold_it_clicked()));
  connect(ui_objection, SIGNAL(clicked()), this, SLOT(on_objection_clicked()));
  connect(ui_take_that, SIGNAL(clicked()), this, SLOT(on_take_that_clicked()));
  connect(ui_custom_objection, SIGNAL(clicked()), this, SLOT(on_custom_objection_clicked()));

  connect(ui_realization, SIGNAL(clicked()), this, SLOT(on_realization_clicked()));
  connect(ui_screenshake, SIGNAL(clicked()), this, SLOT(on_screenshake_clicked()));

  connect(ui_mute, SIGNAL(clicked()), this, SLOT(on_mute_clicked()));

  connect(ui_defense_minus, SIGNAL(clicked()), this, SLOT(on_defense_minus_clicked()));
  connect(ui_defense_plus, SIGNAL(clicked()), this, SLOT(on_defense_plus_clicked()));
  connect(ui_prosecution_minus, SIGNAL(clicked()), this, SLOT(on_prosecution_minus_clicked()));
  connect(ui_prosecution_plus, SIGNAL(clicked()), this, SLOT(on_prosecution_plus_clicked()));

  connect(ui_text_color, SIGNAL(currentIndexChanged(int)), this, SLOT(on_text_color_changed(int)));

  connect(ui_music_slider, SIGNAL(valueChanged(int)), this, SLOT(on_music_slider_moved(int)));
  connect(ui_sfx_slider, SIGNAL(valueChanged(int)), this, SLOT(on_sfx_slider_moved(int)));
  connect(ui_blip_slider, SIGNAL(valueChanged(int)), this, SLOT(on_blip_slider_moved(int)));

  connect(ui_ooc_toggle, SIGNAL(clicked()), this, SLOT(on_ooc_toggle_clicked()));

  connect(ui_music_search, SIGNAL(textChanged(QString)), this, SLOT(on_music_search_edited(QString)));

  connect(ui_witness_testimony, SIGNAL(clicked()), this, SLOT(on_witness_testimony_clicked()));
  connect(ui_cross_examination, SIGNAL(clicked()), this, SLOT(on_cross_examination_clicked()));
  connect(ui_guilty, SIGNAL(clicked()), this, SLOT(on_guilty_clicked()));
  connect(ui_not_guilty, SIGNAL(clicked()), this, SLOT(on_not_guilty_clicked()));

  connect(ui_change_character, SIGNAL(clicked()), this, SLOT(on_change_character_clicked()));
  connect(ui_reload_theme, SIGNAL(clicked()), this, SLOT(on_reload_theme_clicked()));
  connect(ui_call_mod, SIGNAL(clicked()), this, SLOT(on_call_mod_clicked()));
  connect(ui_settings, SIGNAL(clicked()), this, SLOT(on_settings_clicked()));
  connect(ui_announce_casing, SIGNAL(clicked()), this, SLOT(on_announce_casing_clicked()));
  connect(ui_switch_area_music, SIGNAL(clicked()), this, SLOT(on_switch_area_music_clicked()));

  connect(ui_pre, SIGNAL(clicked()), this, SLOT(on_pre_clicked()));
  connect(ui_flip, SIGNAL(clicked()), this, SLOT(on_flip_clicked()));
  connect(ui_additive, SIGNAL(clicked()), this, SLOT(on_additive_clicked()));
  connect(ui_guard, SIGNAL(clicked()), this, SLOT(on_guard_clicked()));
  connect(ui_casing, SIGNAL(clicked()), this, SLOT(on_casing_clicked()));

  connect(ui_showname_enable, SIGNAL(clicked()), this, SLOT(on_showname_enable_clicked()));

  connect(ui_pair_button, SIGNAL(clicked()), this, SLOT(on_pair_clicked()));
  connect(ui_pair_list, SIGNAL(clicked(QModelIndex)), this, SLOT(on_pair_list_clicked(QModelIndex)));
  connect(ui_pair_offset_spinbox, SIGNAL(valueChanged(int)), this, SLOT(on_pair_offset_changed(int)));

  connect(ui_evidence_button, SIGNAL(clicked()), this, SLOT(on_evidence_button_clicked()));

  set_widgets();

  set_char_select();
}

void Courtroom::set_mute_list()
{
  mute_map.clear();

  //maps which characters are muted based on cid, none are muted by default
  for (int n_cid = 0 ; n_cid < char_list.size() ; n_cid++)
  {
    mute_map.insert(n_cid, false);
  }

  QStringList sorted_mute_list;

  for (char_type i_char : char_list)
    sorted_mute_list.append(i_char.name);

  sorted_mute_list.sort();

  for (QString i_name : sorted_mute_list)
  {
    //mute_map.insert(i_name, false);
    ui_mute_list->addItem(i_name);
  }
}

void Courtroom::set_pair_list()
{
  QStringList sorted_pair_list;

  for (char_type i_char : char_list)
    sorted_pair_list.append(i_char.name);

  sorted_pair_list.sort();

  for (QString i_name : sorted_pair_list)
  {
    ui_pair_list->addItem(i_name);
  }
}

void Courtroom::set_widgets()
{
  blip_rate = ao_app->read_blip_rate();
  blank_blip = ao_app->get_blank_blip();

  QString filename = "courtroom_design.ini";

  pos_size_type f_courtroom = ao_app->get_element_dimensions("courtroom", filename);

  if (f_courtroom.width < 0 || f_courtroom.height < 0)
  {
    qDebug() << "W: did not find courtroom width or height in " << filename;

    this->resize(714, 668);
  }
  else
  {
    m_courtroom_width = f_courtroom.width;
    m_courtroom_height = f_courtroom.height;

    this->resize(f_courtroom.width, f_courtroom.height);
  }

  set_fonts();

  ui_background->move(0, 0);
  ui_background->resize(m_courtroom_width, m_courtroom_height);
  ui_background->set_image("courtroombackground");

  set_size_and_pos(ui_viewport, "viewport");

  // If there is a point to it, show all CCCC features.
  // We also do this this soon so that set_size_and_pos can hide them all later, if needed.
  if (ao_app->cccc_ic_support_enabled)
  {
    ui_pair_button->show();
    ui_pre_non_interrupt->show();
    ui_showname_enable->show();
    ui_ic_chat_name->show();
    ui_ic_chat_name->setEnabled(true);
  }
  else
  {
    ui_pair_button->hide();
    ui_pre_non_interrupt->hide();
    ui_showname_enable->hide();
    ui_ic_chat_name->hide();
    ui_ic_chat_name->setEnabled(false);
  }

  if (ao_app->casing_alerts_enabled)
  {
    ui_announce_casing->show();
  }
  else
  {
    ui_announce_casing->hide();
  }

  // We also show the non-server-dependent client additions.
  // Once again, if the theme can't display it, set_move_and_pos will catch them.
  ui_settings->show();

  ui_vp_background->move(0, 0);
  ui_vp_background->resize(ui_viewport->width(), ui_viewport->height());

  ui_vp_speedlines->move(0, 0);
  ui_vp_speedlines->combo_resize(ui_viewport->width(), ui_viewport->height());

  ui_vp_player_char->move(0, 0);
  ui_vp_player_char->combo_resize(ui_viewport->width(), ui_viewport->height());

  ui_vp_sideplayer_char->move(0, 0);
  ui_vp_sideplayer_char->combo_resize(ui_viewport->width(), ui_viewport->height());

  //the AO2 desk element
  ui_vp_desk->move(0, 0);
  ui_vp_desk->resize(ui_viewport->width(), ui_viewport->height());

  //the size of the ui_vp_legacy_desk element relies on various factors and is set in set_scene()

  double y_modifier = 147.0 / 192.0;
  int final_y = static_cast<int>(y_modifier * ui_viewport->height());
  ui_vp_legacy_desk->move(0, final_y);
  ui_vp_legacy_desk->hide();

  ui_vp_evidence_display->move(0, 0);
  ui_vp_evidence_display->resize(ui_viewport->width(), ui_viewport->height());

  set_size_and_pos(ui_vp_showname, "showname");

  set_size_and_pos(ui_vp_message, "message");
  //We detached the text as parent from the chatbox so it doesn't get affected by the screenshake.
  ui_vp_message->move(ui_vp_message->x() + ui_vp_chatbox->x(), ui_vp_message->y() + ui_vp_chatbox->y());
  ui_vp_message->setTextInteractionFlags(Qt::NoTextInteraction);
//  ui_vp_message->setStyleSheet("background-color: rgba(0, 0, 0, 0);"
//                               "color: white");

  ui_vp_chat_arrow->move(0, 0);
  pos_size_type design_ini_result = ao_app->get_element_dimensions("chat_arrow", "courtroom_design.ini");

  if (design_ini_result.width < 0 || design_ini_result.height < 0)
  {
    qDebug() << "W: could not find \"chat_arrow\" in courtroom_design.ini";
    ui_vp_chat_arrow->hide();
  }
  else
  {
    ui_vp_chat_arrow->move(design_ini_result.x, design_ini_result.y);
    ui_vp_chat_arrow->combo_resize(design_ini_result.width, design_ini_result.height);
  }

  ui_vp_testimony->move(ui_viewport->x(), ui_viewport->y());
  ui_vp_testimony->combo_resize(ui_viewport->width(), ui_viewport->height());

  ui_vp_effect->move(ui_viewport->x(), ui_viewport->y());
  ui_vp_effect->combo_resize(ui_viewport->width(), ui_viewport->height());

  ui_vp_wtce->move(ui_viewport->x(), ui_viewport->y());
  ui_vp_wtce->combo_resize(ui_viewport->width(), ui_viewport->height());

  ui_vp_objection->move(ui_viewport->x(), ui_viewport->y());
  ui_vp_objection->combo_resize(ui_viewport->width(), ui_viewport->height());

  set_size_and_pos(ui_ic_chatlog, "ic_chatlog");

  set_size_and_pos(ui_ms_chatlog, "ms_chatlog");

  set_size_and_pos(ui_server_chatlog, "server_chatlog");

  set_size_and_pos(ui_mute_list, "mute_list");
  ui_mute_list->hide();

  set_size_and_pos(ui_pair_list, "pair_list");
  ui_pair_list->hide();
  ui_pair_list->setToolTip(tr("Select a character you wish to pair with."));
  set_size_and_pos(ui_pair_offset_spinbox, "pair_offset_spinbox");
  ui_pair_offset_spinbox->hide();
  ui_pair_offset_spinbox->setToolTip(tr("Change the percentage offset of your character's position from the center of the screen."));
  set_size_and_pos(ui_pair_button, "pair_button");
  ui_pair_button->set_image("pair_button");
  ui_pair_button->setToolTip(tr("Display the list of characters to pair with."));

  set_size_and_pos(ui_area_list, "music_list");
  set_size_and_pos(ui_music_list, "music_list");

  set_size_and_pos(ui_music_name, "music_name");

  ui_music_display->move(0, 0);
  design_ini_result = ao_app->get_element_dimensions("music_display", "courtroom_design.ini");

  if (design_ini_result.width < 0 || design_ini_result.height < 0)
  {
    qDebug() << "W: could not find \"music_name\" in courtroom_design.ini";
    ui_music_display->hide();
  }
  else
  {
    ui_music_display->move(design_ini_result.x, design_ini_result.y);
    ui_music_display->combo_resize(design_ini_result.width, design_ini_result.height);
  }

  ui_music_display->play("music_display");

  if (is_ao2_bg)
  {
    set_size_and_pos(ui_ic_chat_message, "ao2_ic_chat_message");
    set_size_and_pos(ui_vp_chatbox, "ao2_chatbox");
    set_size_and_pos(ui_ic_chat_name, "ao2_ic_chat_name");
  }
  else
  {
    set_size_and_pos(ui_ic_chat_message, "ic_chat_message");
    set_size_and_pos(ui_vp_chatbox, "chatbox");
    set_size_and_pos(ui_ic_chat_name, "ic_chat_name");
  }

  ui_ic_chat_message->setStyleSheet("QLineEdit{background-color: rgba(100, 100, 100, 255);}");
  ui_ic_chat_name->setStyleSheet("QLineEdit{background-color: rgba(180, 180, 180, 255);}");

  ui_vp_chatbox->set_image("chatblank");
  ui_vp_chatbox->hide();

  ui_muted->resize(ui_ic_chat_message->width(), ui_ic_chat_message->height());
  ui_muted->set_image("muted");
  ui_muted->setToolTip(tr("Oops, you're muted!"));

  set_size_and_pos(ui_ooc_chat_message, "ooc_chat_message");
  ui_ooc_chat_message->setStyleSheet("background-color: rgba(0, 0, 0, 0);");
  ui_ooc_chat_message->setToolTip(tr("Type your message to display in the server chat here."));

  set_size_and_pos(ui_ooc_chat_name, "ooc_chat_name");
  ui_ooc_chat_name->setStyleSheet("background-color: rgba(0, 0, 0, 0);");
  ui_ooc_chat_name->setToolTip(tr("Set your name to display in the server chat."));

  //set_size_and_pos(ui_area_password, "area_password");
  set_size_and_pos(ui_music_search, "music_search");

  set_size_and_pos(ui_emote_dropdown, "emote_dropdown");
  ui_emote_dropdown->setToolTip(tr("Set your character's emote to play on your next message."));

  set_size_and_pos(ui_pos_dropdown, "pos_dropdown");
  ui_pos_dropdown->setToolTip(tr("Set your character's supplementary background."));

  set_size_and_pos(ui_iniswap_dropdown, "iniswap_dropdown");
  ui_iniswap_dropdown->setEditable(true);
  ui_iniswap_dropdown->setInsertPolicy(QComboBox::InsertAtBottom);
  ui_iniswap_dropdown->setToolTip(tr("Set an 'iniswap', or an alternative character folder to refer to from your current character.\n"
                                     "Edit by typing and pressing Enter, [X] to remove. This saves to your base/characters/<charname>/iniswaps.ini"));

  set_size_and_pos(ui_iniswap_remove, "iniswap_remove");
  ui_iniswap_remove->setText("X");
  ui_iniswap_remove->set_image("evidencex");
  ui_iniswap_remove->setToolTip(tr("Remove the currently selected iniswap from the list and return to the original character folder."));

  set_size_and_pos(ui_sfx_dropdown, "sfx_dropdown");
  ui_sfx_dropdown->setEditable(true);
  ui_sfx_dropdown->setInsertPolicy(QComboBox::InsertAtBottom);
  ui_sfx_dropdown->setToolTip(tr("Set a sound effect to play on your next 'Preanim'. Leaving it on Default will use the emote-defined sound (if any).\n"
                                  "Edit by typing and pressing Enter, [X] to remove. This saves to your base/characters/<charname>/soundlist.ini"));

  set_size_and_pos(ui_sfx_remove, "sfx_remove");
  ui_sfx_remove->setText("X");
  ui_sfx_remove->set_image("evidencex");
  ui_sfx_remove->setToolTip(tr("Remove the currently selected iniswap from the list and return to the original character folder."));

  set_size_and_pos(ui_effects_dropdown, "effects_dropdown");
  ui_effects_dropdown->setInsertPolicy(QComboBox::InsertAtBottom);
  ui_effects_dropdown->setToolTip(tr("Choose an effect to play on your next spoken message.\n"
                                     "The effects are defined in your theme/effects/effects.ini. Your character can define custom effects by\n"
                                     "char.ini [Options] category, effects = 'miscname' where it referes to misc/<miscname>/effects.ini to read the effects."));
  //Todo: recode this entire fucking system with these dumbass goddamn ini's why is everything so specifically coded for all these purposes
  //is ABSTRACT CODING not a thing now huh what the FUCK why do I gotta do this pleASE FOR THE LOVE OF GOD SPARE ME FROM THIS FRESH HELL
  //btw i still love coding.
  QPoint p_point = ao_app->get_button_spacing("effects_icon_size", filename);
  ui_effects_dropdown->setIconSize(QSize(p_point.x(), p_point.y()));

  set_size_and_pos(ui_defense_bar, "defense_bar");
  ui_defense_bar->set_image("defensebar" + QString::number(defense_bar_state));

  set_size_and_pos(ui_prosecution_bar, "prosecution_bar");
  ui_prosecution_bar->set_image("prosecutionbar" + QString::number(prosecution_bar_state));

  set_size_and_pos(ui_music_label, "music_label");
  ui_music_label->setText(tr("Music"));
  set_size_and_pos(ui_sfx_label, "sfx_label");
  ui_sfx_label->setText(tr("Sfx"));
  set_size_and_pos(ui_blip_label, "blip_label");
  ui_blip_label->setText(tr("Blips"));

  set_size_and_pos(ui_hold_it, "hold_it");
  ui_hold_it->setText(tr("Hold It!"));
  ui_hold_it->setToolTip(tr("When this is turned on, your next in-character message will be a shout!"));
  ui_hold_it->set_image("holdit");

  set_size_and_pos(ui_objection, "objection");
  ui_objection->setText(tr("Objection!"));
  ui_objection->setToolTip(tr("When this is turned on, your next in-character message will be a shout!"));
  ui_objection->set_image("objection");

  set_size_and_pos(ui_take_that, "take_that");
  ui_take_that->setText(tr("Take That!"));
  ui_take_that->setToolTip(tr("When this is turned on, your next in-character message will be a shout!"));
  ui_take_that->set_image("takethat");

  set_size_and_pos(ui_ooc_toggle, "ooc_toggle");
  ui_ooc_toggle->setText(tr("Server"));
  ui_ooc_toggle->setToolTip(tr("Toggle between server chat and global AO2 chat."));

  set_size_and_pos(ui_witness_testimony, "witness_testimony");
  ui_witness_testimony->set_image("witnesstestimony");
  ui_witness_testimony->setToolTip(tr("This will display the animation in the viewport as soon as it is pressed."));
  set_size_and_pos(ui_cross_examination, "cross_examination");
  ui_cross_examination->set_image("crossexamination");
  ui_cross_examination->setToolTip(tr("This will display the animation in the viewport as soon as it is pressed."));

  set_size_and_pos(ui_guilty, "guilty");
  ui_guilty->setText(tr("Guilty!"));
  ui_guilty->set_image("guilty");
  ui_guilty->setToolTip(tr("This will display the animation in the viewport as soon as it is pressed."));
  set_size_and_pos(ui_not_guilty, "not_guilty");
  ui_not_guilty->set_image("notguilty");
  ui_not_guilty->setToolTip(tr("This will display the animation in the viewport as soon as it is pressed."));

  set_size_and_pos(ui_change_character, "change_character");
  ui_change_character->setText(tr("Change character"));
  ui_change_character->set_image("change_character");
  ui_change_character->setToolTip(tr("Bring up the Character Select Screen and change your character."));

  set_size_and_pos(ui_reload_theme, "reload_theme");
  ui_reload_theme->setText(tr("Reload theme"));
  ui_reload_theme->set_image("reload_theme");
  ui_reload_theme->setToolTip(tr("Refresh the theme and update all of the ui elements to match."));

  set_size_and_pos(ui_call_mod, "call_mod");
  ui_call_mod->setText(tr("Call mod"));
  ui_call_mod->set_image("call_mod");
  ui_call_mod->setToolTip(tr("Request the attention of the current server's moderator."));

  set_size_and_pos(ui_settings, "settings");
  ui_settings->setText(tr("Settings"));
  ui_settings->set_image("settings");
  ui_settings->setToolTip(tr("Allows you to change various aspects of the client."));

  set_size_and_pos(ui_announce_casing, "casing_button");
  ui_announce_casing->setText(tr("Casing"));
  ui_announce_casing->set_image("casing_button");
  ui_announce_casing->setToolTip(tr("An interface to help you announce a case (you have to be a CM first to be able to announce cases)"));

  set_size_and_pos(ui_switch_area_music, "switch_area_music");
  ui_switch_area_music->setText(tr("A/M"));
  ui_switch_area_music->set_image("switch_area_music");
  ui_switch_area_music->setToolTip(tr("Switch between Areas and Music lists"));

  set_size_and_pos(ui_pre, "pre");
  ui_pre->setText(tr("Preanim"));
  ui_pre->setToolTip(tr("Play a single-shot animation as defined by the emote when checked."));

  set_size_and_pos(ui_pre_non_interrupt, "pre_no_interrupt");
  ui_pre_non_interrupt->setToolTip(tr("If preanim is checked, display the input text immediately as the animation plays concurrently."));

  set_size_and_pos(ui_flip, "flip");
  ui_flip->setToolTip(tr("Mirror your character's emotes when checked."));

  set_size_and_pos(ui_additive, "additive");
  ui_additive->setToolTip(tr("Add text to your last spoken message when checked."));

  set_size_and_pos(ui_guard, "guard");
  ui_guard->setToolTip(tr("Do not listen to mod calls when checked, preventing them from playing sounds or focusing attention on the window."));

  set_size_and_pos(ui_casing, "casing");
  ui_casing->setToolTip(tr("Lets you receive case alerts when enabled.\n"
                           "(You can set your preferences in the Settings!)"));

  set_size_and_pos(ui_showname_enable, "showname_enable");
  ui_showname_enable->setToolTip(tr("Display customized shownames for all users when checked."));

  set_size_and_pos(ui_custom_objection, "custom_objection");
  ui_custom_objection->setText(tr("Custom Shout!"));
  ui_custom_objection->set_image("custom");
  ui_custom_objection->setToolTip(tr("This will display the custom character-defined animation in the viewport as soon as it is pressed.\n"
                                     "To make one, your character's folder must contain custom.[webp/apng/gif/png] and custom.wav"));

  set_size_and_pos(ui_realization, "realization");
  ui_realization->set_image("realization");
  ui_realization->setToolTip(tr("Play realization sound and animation in the viewport on the next spoken message when checked."));

  set_size_and_pos(ui_screenshake, "screenshake");
  ui_screenshake->set_image("screenshake");
  ui_screenshake->setToolTip(tr("Shake the screen on next spoken message when checked."));

  set_size_and_pos(ui_mute, "mute_button");
  ui_mute->setText("Mute");
  ui_mute->set_image("mute");
  ui_mute->setToolTip(tr("Display the list of character folders you wish to mute."));

  set_size_and_pos(ui_defense_plus, "defense_plus");
  ui_defense_plus->set_image("defplus");
  ui_defense_plus->setToolTip(tr("Increase the health bar."));

  set_size_and_pos(ui_defense_minus, "defense_minus");
  ui_defense_minus->set_image("defminus");
  ui_defense_minus->setToolTip(tr("Decrease the health bar."));

  set_size_and_pos(ui_prosecution_plus, "prosecution_plus");
  ui_prosecution_plus->set_image("proplus");
  ui_prosecution_plus->setToolTip(tr("Increase the health bar."));

  set_size_and_pos(ui_prosecution_minus, "prosecution_minus");
  ui_prosecution_minus->set_image("prominus");
  ui_prosecution_minus->setToolTip(tr("Decrease the health bar."));

  set_size_and_pos(ui_text_color, "text_color");
  ui_text_color->setToolTip(tr("Change the text color of the spoken message."));
  set_text_color_dropdown();

  set_size_and_pos(ui_music_slider, "music_slider");
  set_size_and_pos(ui_sfx_slider, "sfx_slider");
  set_size_and_pos(ui_blip_slider, "blip_slider");

  set_size_and_pos(ui_evidence_button, "evidence_button");
  ui_evidence_button->set_image("evidencebutton");
  ui_evidence_button->setToolTip(tr("Bring up the Evidence screen."));

  set_size_and_pos(ui_evidence, "evidence_background");
  ui_evidence->set_image("evidencebackground");

  set_size_and_pos(ui_evidence_name, "evidence_name");

  set_size_and_pos(ui_evidence_buttons, "evidence_buttons");

  set_size_and_pos(ui_evidence_left, "evidence_left");
  ui_evidence_left->set_image("arrow_left");

  set_size_and_pos(ui_evidence_right, "evidence_right");
  ui_evidence_right->set_image("arrow_right");

  set_size_and_pos(ui_evidence_present, "evidence_present");
  ui_evidence_present->set_image("present_disabled");

  set_size_and_pos(ui_evidence_overlay, "evidence_overlay");
  ui_evidence_overlay->set_image("evidenceoverlay");

  set_size_and_pos(ui_evidence_delete, "evidence_delete");
  ui_evidence_delete->set_image("deleteevidence");

  set_size_and_pos(ui_evidence_image_name, "evidence_image_name");

  set_size_and_pos(ui_evidence_image_button, "evidence_image_button");

  set_size_and_pos(ui_evidence_x, "evidence_x");
  ui_evidence_x->set_image("evidencex");

  set_size_and_pos(ui_evidence_description, "evidence_description");

  ui_selector->set_image("char_selector");
  ui_selector->hide();

  set_size_and_pos(ui_back_to_lobby, "back_to_lobby");
  ui_back_to_lobby->setText(tr("Back to Lobby"));
  ui_back_to_lobby->setToolTip(tr("Return back to the server list."));

  set_size_and_pos(ui_char_password, "char_password");

  set_size_and_pos(ui_char_buttons, "char_buttons");

  set_size_and_pos(ui_char_select_left, "char_select_left");
  ui_char_select_left->set_image("arrow_left");

  set_size_and_pos(ui_char_select_right, "char_select_right");
  ui_char_select_right->set_image("arrow_right");

  set_size_and_pos(ui_spectator, "spectator");
  ui_spectator->setToolTip(tr("Become a spectator. You won't be able to interact with the in-character screen."));

  refresh_evidence();
  refresh_emotes();
}

void Courtroom::set_fonts()
{
  set_font(ui_vp_showname, "", "showname");
  set_font(ui_vp_message, "",  "message");
  set_font(ui_ic_chatlog, "", "ic_chatlog");
  set_font(ui_ms_chatlog, "", "ms_chatlog");
  set_font(ui_server_chatlog, "", "server_chatlog");
  set_font(ui_music_list, "", "music_list");
  set_font(ui_area_list, "", "area_list");
  set_font(ui_music_name, "", "music_name");

  set_dropdowns();
}

void Courtroom::set_font(QWidget *widget, QString class_name, QString p_identifier)
{
  QString design_file = "courtroom_fonts.ini";
  int f_weight = ao_app->get_font_size(p_identifier, design_file);
  QString font_name = ao_app->get_font_name(p_identifier + "_font", design_file);
  QColor f_color = ao_app->get_color(p_identifier + "_color", design_file);

  bool bold = ao_app->get_font_size(p_identifier + "_bold", design_file) == 1; // is the font bold or not?
  this->set_qfont(widget, class_name, QFont(font_name, f_weight), f_color, bold);
}

void Courtroom::set_qfont(QWidget *widget, QString class_name, QFont font, QColor f_color, bool bold)
{
  if(class_name == "")
    class_name = widget->metaObject()->className();
  widget->setFont(font);

  QString is_bold = "";
  if(bold) is_bold = "font: bold;";

  QString style_sheet_string = class_name + " { background-color: rgba(0, 0, 0, 0);\n" +
                               "color: rgba(" +
                               QString::number(f_color.red()) + ", " +
                               QString::number(f_color.green()) + ", " +
                               QString::number(f_color.blue()) + ", 255);\n" + is_bold + "}";
  widget->setStyleSheet(style_sheet_string);
}

void Courtroom::set_dropdown(QWidget *widget)
{
  QString f_file = "courtroom_stylesheets.css";
  QString style_sheet_string = ao_app->get_stylesheet(f_file);
  if (style_sheet_string != "")
    widget->setStyleSheet(style_sheet_string);
}

void Courtroom::set_dropdowns()
{
  set_dropdown(this); //EXPERIMENTAL - Read the style-sheet as-is for maximum memeage
//  set_dropdown(ui_text_color, "[TEXT COLOR]");
//  set_dropdown(ui_pos_dropdown, "[POS DROPDOWN]");
//  set_dropdown(ui_emote_dropdown, "[EMOTE DROPDOWN]");
//  set_dropdown(ui_mute_list, "[MUTE LIST]");
}

void Courtroom::set_window_title(QString p_title)
{
  this->setWindowTitle(p_title);
}

void Courtroom::set_size_and_pos(QWidget *p_widget, QString p_identifier)
{
  QString filename = "courtroom_design.ini";

  pos_size_type design_ini_result = ao_app->get_element_dimensions(p_identifier, filename);

  if (design_ini_result.width < 0 || design_ini_result.height < 0)
  {
    qDebug() << "W: could not find \"" << p_identifier << "\" in " << filename;
    p_widget->hide();
  }
  else
  {
    p_widget->move(design_ini_result.x, design_ini_result.y);
    p_widget->resize(design_ini_result.width, design_ini_result.height);
  }
}

void Courtroom::set_taken(int n_char, bool p_taken)
{
  if (n_char >= char_list.size())
  {
    qDebug() << "W: set_taken attempted to set an index bigger than char_list size";
    return;
  }

  char_type f_char;
  f_char.name = char_list.at(n_char).name;
  f_char.description = char_list.at(n_char).description;
  f_char.taken = p_taken;
  f_char.evidence_string = char_list.at(n_char).evidence_string;

  char_list.replace(n_char, f_char);
}

QPoint Courtroom::get_theme_pos(QString p_identifier)
{
  QString filename = "courtroom_design.ini";

  pos_size_type design_ini_result = ao_app->get_element_dimensions(p_identifier, filename);

  if (design_ini_result.width < 0 || design_ini_result.height < 0)
  {
    qDebug() << "W: could not find \"" << p_identifier << "\" in " << filename;
    return QPoint(0,0);
  }
  else
  {
    return QPoint(design_ini_result.x, design_ini_result.y);
  }
}

void Courtroom::done_received()
{
  m_cid = -1;

  music_player->set_volume(0);
  sfx_player->set_volume(0);
  objection_player->set_volume(0);
  blip_player->set_volume(0);

  set_char_select_page();

  set_mute_list();
  set_pair_list();

  set_char_select();

  show();

  ui_spectator->show();
}

void Courtroom::set_background(QString p_background)
{
  ui_vp_testimony->stop();
  current_background = p_background;

  is_ao2_bg = file_exists(ao_app->get_static_image_suffix(ao_app->get_background_path("defensedesk"))) &&
              file_exists(ao_app->get_static_image_suffix(ao_app->get_background_path("prosecutiondesk"))) &&
              file_exists(ao_app->get_static_image_suffix(ao_app->get_background_path("stand")));

  if (is_ao2_bg)
  {
    set_size_and_pos(ui_vp_chatbox, "ao2_chatbox");
    set_size_and_pos(ui_ic_chat_message, "ao2_ic_chat_message");
  }
  else
  {
    set_size_and_pos(ui_vp_chatbox, "chatbox");
    set_size_and_pos(ui_ic_chat_message, "ic_chat_message");
  }

  ui_vp_speedlines->stop();
  ui_vp_player_char->stop();
  ui_vp_sideplayer_char->stop();
  ui_vp_effect->stop();
  ui_vp_message->hide();
  ui_vp_chatbox->hide();
  set_scene(QString::number(ao_app->get_desk_mod(current_char, current_emote)), current_side);
}

void Courtroom::set_side(QString p_side)
{
  if (p_side == "")
    current_side = ao_app->get_char_side(current_char);
  else
    current_side = p_side;

  for (int i = 0; i < ui_pos_dropdown->count(); ++i)
  {
    QString pos = ui_pos_dropdown->itemText(i);
    if (pos == current_side)
    {
      //Block the signals to prevent setCurrentIndex from triggering a pos change
      ui_pos_dropdown->blockSignals(true);

      //Set the index on dropdown ui element to let you know what pos you're on right now
      ui_pos_dropdown->setCurrentIndex(i);

      //Unblock the signals so the element can be used for setting pos again
      ui_pos_dropdown->blockSignals(false);

      //alright we dun, jobs done here boyos
      break;
    }
  }
}


void Courtroom::update_character(int p_cid)
{
  bool newchar = m_cid != p_cid;

  m_cid = p_cid;

  QString f_char;

  if (m_cid == -1)
  {
    if (ao_app->is_discord_enabled())
      ao_app->discord->state_spectate();
    f_char = "";
  }
  else
  {
    f_char = ao_app->get_char_name(char_list.at(m_cid).name);

    if (ao_app->is_discord_enabled())
      ao_app->discord->state_character(f_char.toStdString());
  }

  current_char = f_char;

  current_emote_page = 0;
  current_emote = 0;

  if (m_cid == -1)
    ui_emotes->hide();
  else
    ui_emotes->show();

  set_emote_page();
  set_emote_dropdown();

  set_sfx_dropdown();
  set_effects_dropdown();

  QString side = current_side;
  if (side == "")
    side = ao_app->get_char_side(current_char);

  if (side == "jud")
  {
    ui_witness_testimony->show();
    ui_cross_examination->show();
    ui_not_guilty->show();
    ui_guilty->show();
    ui_defense_minus->show();
    ui_defense_plus->show();
    ui_prosecution_minus->show();
    ui_prosecution_plus->show();
  }
  else
  {
    ui_witness_testimony->hide();
    ui_cross_examination->hide();
    ui_guilty->hide();
    ui_not_guilty->hide();
    ui_defense_minus->hide();
    ui_defense_plus->hide();
    ui_prosecution_minus->hide();
    ui_prosecution_plus->hide();
  }

  if (ao_app->custom_objection_enabled &&
      (file_exists(ao_app->get_character_path(current_char, "custom.gif")) ||
      file_exists(ao_app->get_character_path(current_char, "custom.apng"))) &&
      file_exists(ao_app->get_character_path(current_char, "custom.wav")))
    ui_custom_objection->show();
  else
    ui_custom_objection->hide();

  ui_char_select_background->hide();
  ui_ic_chat_message->setEnabled(m_cid != -1);
  ui_ic_chat_message->setFocus();

  if (newchar)
    set_iniswap_dropdown();
}

void Courtroom::enter_courtroom()
{
  set_widgets();

  current_evidence_page = 0;
  current_evidence = 0;

  set_evidence_page();

  if (ao_app->flipping_enabled)
    ui_flip->show();
  else
    ui_flip->hide();

  if (ao_app->additive_enabled)
    ui_additive->show();
  else
    ui_additive->hide();

  if (ao_app->casing_alerts_enabled)
    ui_casing->show();
  else
    ui_casing->hide();

  list_music();
  list_areas();

  music_player->set_volume(ui_music_slider->value(), 0); //set music
  //Set the ambience and other misc. music layers
  for (int i = 1; i < music_player->m_channelmax; ++i)
  {
    music_player->set_volume(ui_sfx_slider->value(), i);
  }
  sfx_player->set_volume(ui_sfx_slider->value());
  objection_player->set_volume(ui_sfx_slider->value());
  blip_player->set_volume(ui_blip_slider->value());

  ui_vp_testimony->stop();
  //ui_server_chatlog->setHtml(ui_server_chatlog->toHtml());
}

void Courtroom::list_music()
{
  ui_music_list->clear();
  music_row_to_number.clear();

  QString f_file = "courtroom_design.ini";

  QBrush found_brush(ao_app->get_color("found_song_color", f_file));
  QBrush missing_brush(ao_app->get_color("missing_song_color", f_file));

  int n_listed_songs = 0;

  QTreeWidgetItem *parent = nullptr;
  for (int n_song = 0 ; n_song < music_list.size() ; ++n_song)
  {
    QString i_song = music_list.at(n_song);
    QString i_song_listname = i_song.left(i_song.lastIndexOf("."));

    QTreeWidgetItem *treeItem;
    if (i_song_listname != i_song && parent != nullptr) //not a category, parent exists
      treeItem = new QTreeWidgetItem(parent);
    else
      treeItem = new QTreeWidgetItem(ui_music_list);
    treeItem->setText(0, i_song);
    music_row_to_number.append(n_song);

    QString song_path = ao_app->get_music_path(i_song);

    if (file_exists(song_path))
      treeItem->setBackground(0, found_brush);
    else
      treeItem->setBackground(0, missing_brush);

    if (i_song_listname == i_song) //Not supposed to be a song to begin with - a category?
      parent = treeItem;
    ++n_listed_songs;

    if (ui_music_search->text() != "")
      treeItem->setHidden(true);
  }

  if (ui_music_search->text() != "")
  {
    QList<QTreeWidgetItem*> clist = ui_music_list->findItems(ui_music_search->text(), Qt::MatchContains|Qt::MatchRecursive, 0);
    foreach(QTreeWidgetItem* item, clist)
    {
      if (item->parent() != nullptr) //So the category shows up too
        item->parent()->setHidden(false);
      item->setHidden(false);
    }
  }
  ui_music_list->expandAll(); //Workaround, it needs to preserve the "expanded categories" due to list music being updated constantly by some servers
}

void Courtroom::list_areas()
{
  ui_area_list->clear();
  area_row_to_number.clear();

  QString f_file = "courtroom_design.ini";

  QBrush free_brush(ao_app->get_color("area_free_color", f_file));
  QBrush lfp_brush(ao_app->get_color("area_lfp_color", f_file));
  QBrush casing_brush(ao_app->get_color("area_casing_color", f_file));
  QBrush recess_brush(ao_app->get_color("area_recess_color", f_file));
  QBrush rp_brush(ao_app->get_color("area_rp_color", f_file));
  QBrush gaming_brush(ao_app->get_color("area_gaming_color", f_file));
  QBrush locked_brush(ao_app->get_color("area_locked_color", f_file));

  int n_listed_areas = 0;

  for (int n_area = 0 ; n_area < area_list.size() ; ++n_area)
  {
    QString i_area = "";
    i_area.append(area_list.at(n_area));

    if (ao_app->arup_enabled)
    {
      i_area.prepend("[" + QString::number(n_area) + "] "); //Give it the index

      i_area.append("\n  ");

      i_area.append(arup_statuses.at(n_area));
      i_area.append(" | CM: ");
      i_area.append(arup_cms.at(n_area));

      i_area.append("\n  ");

      i_area.append(QString::number(arup_players.at(n_area)));
      i_area.append(" users | ");

      i_area.append(arup_locks.at(n_area));
    }

    if (i_area.toLower().contains(ui_music_search->text().toLower()))
    {
      ui_area_list->addItem(i_area);
      area_row_to_number.append(n_area);

      if (ao_app->arup_enabled)
      {
        // Coloring logic here.
        ui_area_list->item(n_listed_areas)->setBackground(free_brush);
        if (arup_locks.at(n_area) == "LOCKED")
        {
            ui_area_list->item(n_listed_areas)->setBackground(locked_brush);
        }
        else
        {
            if (arup_statuses.at(n_area) == "LOOKING-FOR-PLAYERS")
                ui_area_list->item(n_listed_areas)->setBackground(lfp_brush);
            else if (arup_statuses.at(n_area) == "CASING")
                ui_area_list->item(n_listed_areas)->setBackground(casing_brush);
            else if (arup_statuses.at(n_area) == "RECESS")
                ui_area_list->item(n_listed_areas)->setBackground(recess_brush);
            else if (arup_statuses.at(n_area) == "RP")
                ui_area_list->item(n_listed_areas)->setBackground(rp_brush);
            else if (arup_statuses.at(n_area) == "GAMING")
                ui_area_list->item(n_listed_areas)->setBackground(gaming_brush);
        }
      }
      else
      {
        ui_area_list->item(n_listed_areas)->setBackground(free_brush);
      }

      ++n_listed_areas;
    }
  }
}

void Courtroom::append_ms_chatmessage(QString f_name, QString f_message)
{
  ui_ms_chatlog->append_chatmessage(f_name, f_message, ao_app->get_color("ms_chatlog_sender_color", "courtroom_fonts.ini").name());
}

void Courtroom::append_server_chatmessage(QString p_name, QString p_message, QString p_color)
{
  QString color = "#000000";

  if (p_color == "0")
    color = ao_app->get_color("ms_chatlog_sender_color", "courtroom_fonts.ini").name();
  if (p_color == "1")
    color = ao_app->get_color("server_chatlog_sender_color", "courtroom_fonts.ini").name();
  if(p_message == "Logged in as a moderator.")
  {
    ui_guard->show();
    append_server_chatmessage("CLIENT", "You were granted the Disable Modcalls button.", "1");
  }

  ui_server_chatlog->append_chatmessage(p_name, p_message, color);
}

void Courtroom::on_chat_return_pressed()
{
  if (ui_ic_chat_message->text() == "" || is_muted)
    return;

  if ((anim_state < 3 || text_state < 2) &&
      objection_state == 0)
    return;

  //MS#
  //deskmod#
  //pre-emote#
  //character#
  //emote#
  //message#
  //side#
  //sfx-name#
  //emote_modifier#
  //char_id#
  //sfx_delay#
  //objection_modifier#
  //evidence#
  //placeholder#
  //realization#
  //text_color#%

  // Additionally, in our case:

  //showname#
  //other_charid#
  //self_offset#
  //noninterrupting_preanim#%

  QStringList packet_contents;

  QString f_side = current_side;
  if (f_side == "")
    f_side = ao_app->get_char_side(current_char);

  QString f_desk_mod = "chat";

  if (ao_app->desk_mod_enabled)
  {
    f_desk_mod = QString::number(ao_app->get_desk_mod(current_char, current_emote));
    if (f_desk_mod == "-1")
      f_desk_mod = "chat";
  }

  packet_contents.append(f_desk_mod);

  packet_contents.append(ao_app->get_pre_emote(current_char, current_emote));

  packet_contents.append(current_char);

  packet_contents.append(ao_app->get_emote(current_char, current_emote));

  packet_contents.append(ui_ic_chat_message->text());

  packet_contents.append(f_side);

  packet_contents.append(get_char_sfx());

  int f_emote_mod = ao_app->get_emote_mod(current_char, current_emote);

  //needed or else legacy won't understand what we're saying
  if (objection_state > 0)
  {
    if (ui_pre->isChecked())
    {
      if (f_emote_mod == 5)
        f_emote_mod = 6;
      else
        f_emote_mod = 2;
    }
  }
  else if (ui_pre->isChecked() and !ui_pre_non_interrupt->isChecked())
  {
    if (f_emote_mod == 0)
      f_emote_mod = 1;
    else if (f_emote_mod == 5 && ao_app->prezoom_enabled)
      f_emote_mod = 4;
  }
  else
  {
    if (f_emote_mod == 1)
      f_emote_mod = 0;
    else if (f_emote_mod == 4)
      f_emote_mod = 5;
  }

  packet_contents.append(QString::number(f_emote_mod));
  packet_contents.append(QString::number(m_cid));

  packet_contents.append(QString::number(get_char_sfx_delay()));

  QString f_obj_state;

  if ((objection_state == 4 && !ao_app->custom_objection_enabled) ||
    (objection_state < 0))
    f_obj_state = "0";
  else
    f_obj_state = QString::number(objection_state);

  packet_contents.append(f_obj_state);

  if (is_presenting_evidence)
    //the evidence index is shifted by 1 because 0 is no evidence per legacy standards
    //besides, older clients crash if we pass -1
    packet_contents.append(QString::number(current_evidence + 1));
  else
    packet_contents.append("0");

  QString f_flip;

  if (ao_app->flipping_enabled)
  {
    if (ui_flip->isChecked())
      f_flip = "1";
    else
      f_flip = "0";
  }
  else
    f_flip = QString::number(m_cid);

  packet_contents.append(f_flip);

  packet_contents.append(QString::number(realization_state));

  QString f_text_color;

  if (text_color < 0)
    f_text_color = "0";
  else if (text_color > 8)
    f_text_color = "0";
  else
    f_text_color = QString::number(text_color);

  packet_contents.append(f_text_color);

  // If the server we're on supports CCCC stuff, we should use it!
  if (ao_app->cccc_ic_support_enabled)
  {
    // If there is a showname entered, use that -- else, just send an empty packet-part.
    if (!ui_ic_chat_name->text().isEmpty())
    {
      packet_contents.append(ui_ic_chat_name->text());
    }
    else
    {
      packet_contents.append("");
    }

    // Similarly, we send over whom we're paired with, unless we have chosen ourselves.
    // Or a charid of -1 or lower, through some means.
    if (other_charid > -1 && other_charid != m_cid)
    {
      packet_contents.append(QString::number(other_charid));
      packet_contents.append(QString::number(offset_with_pair));
    }
    else
    {
      packet_contents.append("-1");
      packet_contents.append("0");
    }

    // Finally, we send over if we want our pres to not interrupt.
    if (ui_pre_non_interrupt->isChecked() && ui_pre->isChecked())
    {
      packet_contents.append("1");
    }
    else
    {
      packet_contents.append("0");
    }
  }

  // If the server we're on supports Looping SFX and Screenshake, use it if the emote uses it.
  if (ao_app->looping_sfx_support_enabled)
  {
      packet_contents.append("0"); //ao_app->get_sfx_looping(current_char, current_emote));
      packet_contents.append(QString::number(screenshake_state));

      QString pre_emote = ao_app->get_pre_emote(current_char, current_emote);
      QString emote = ao_app->get_emote(current_char, current_emote);
      QStringList emotes_to_check = {pre_emote, "(b)" + emote, "(a)" + emote};
      QStringList effects_to_check = {"_FrameScreenshake", "_FrameRealization", "_FrameSFX"};

      foreach (QString f_effect, effects_to_check)
      {
        QString packet;
        foreach (QString f_emote, emotes_to_check)
        {
          packet += f_emote;
          if (ao_app->is_frame_network_enabled())
          {
            QString sfx_frames = ao_app->read_ini_tags(ao_app->get_character_path(current_char, "char.ini"), f_emote.append(f_effect)).join("|");
            if (sfx_frames != "")
              packet += "|" + sfx_frames;
          }
          packet += "^";
        }
        packet_contents.append(packet);
      }
  }

  if (ao_app->additive_enabled)
  {
    packet_contents.append(ui_additive->isChecked() ? "1" : "0");
  }
  if (ao_app->effects_enabled)
  {
    QString fx_sound = ao_app->get_effect_sound(effect, current_char);
    packet_contents.append(effect + "|" + fx_sound);
    ui_effects_dropdown->setCurrentIndex(0);
    effect = "";
  }

  ao_app->send_server_packet(new AOPacket("MS", packet_contents));
}

void Courtroom::handle_chatmessage(QStringList *p_contents)
{
  // Instead of checking for whether a message has at least chatmessage_size
  // amount of packages, we'll check if it has at least 15.
  // That was the original chatmessage_size.
  if (p_contents->size() < 15)
    return;

  for (int n_string = 0 ; n_string < chatmessage_size ; ++n_string)
  {
    //m_chatmessage[n_string] = p_contents->at(n_string);

    // Note that we have added stuff that vanilla clients and servers simply won't send.
    // So now, we have to check if the thing we want even exists amongst the packet's content.
    // We also have to check if the server even supports CCCC's IC features, or if it's just japing us.
    // Also, don't forget! A size 15 message will have indices from 0 to 14.
    if (n_string < p_contents->size() &&
       (n_string < 15 || ao_app->cccc_ic_support_enabled))
    {
      m_chatmessage[n_string] = p_contents->at(n_string);
    }
    else
    {
      m_chatmessage[n_string] = "";
    }
  }

  int f_char_id = m_chatmessage[CHAR_ID].toInt();

  if (f_char_id < 0 || f_char_id >= char_list.size())
    return;

  if (mute_map.value(m_chatmessage[CHAR_ID].toInt()))
    return;

  QString f_showname;
  if (m_chatmessage[SHOWNAME].isEmpty() || !ui_showname_enable->isChecked())
  {
      f_showname = ao_app->get_showname(char_list.at(f_char_id).name);
  }
  else
  {
      f_showname = m_chatmessage[SHOWNAME];
  }

  if(f_showname.trimmed().isEmpty()) //Pure whitespace showname, get outta here.
    f_showname = m_chatmessage[CHAR_NAME];


  QString f_message = f_showname + ": " + m_chatmessage[MESSAGE] + '\n';

  if (f_message == previous_ic_message)
    return;

  //Stop the chat arrow from animating
  ui_vp_chat_arrow->stop();

  text_state = 0;
  anim_state = 0;
  ui_vp_objection->stop();
  chat_tick_timer->stop();
  ui_vp_evidence_display->reset();

  m_chatmessage[MESSAGE].remove("\n"); //Remove undesired newline chars

  chatmessage_is_empty = m_chatmessage[MESSAGE] == " " || m_chatmessage[MESSAGE] == "";

  //Hey, our message showed up! Cool!
  if (m_chatmessage[MESSAGE] == ui_ic_chat_message->text().remove("\n") && m_chatmessage[CHAR_ID].toInt() == m_cid)
  {
    ui_ic_chat_message->clear();
    if (ui_additive->isChecked())
      ui_ic_chat_message->insert(" ");
    objection_state = 0;
    realization_state = 0;
    screenshake_state = 0;
    is_presenting_evidence = false;
    ui_pre->setChecked(false);
    ui_hold_it->set_image("holdit");
    ui_objection->set_image("objection");
    ui_take_that->set_image("takethat");
    ui_custom_objection->set_image("custom");
    ui_realization->set_image("realization");
    ui_screenshake->set_image("screenshake");
    ui_evidence_present->set_image("present_disabled");
  }

  //Let the server handle actually checking if they're allowed to do this.
  is_additive = m_chatmessage[ADDITIVE].toInt() == 1;

  chatlogpiece* temp = new chatlogpiece(ao_app->get_showname(char_list.at(f_char_id).name), f_showname, ": " + m_chatmessage[MESSAGE], false);
  ic_chatlog_history.append(*temp);
  ao_app->append_to_file(temp->get_full(), ao_app->log_filename, true);

  while(ic_chatlog_history.size() > log_maximum_blocks && log_maximum_blocks > 0)
  {
    ic_chatlog_history.removeFirst();
  }

  append_ic_text(m_chatmessage[MESSAGE], f_showname);

  previous_ic_message = f_message;

  int objection_mod = m_chatmessage[OBJECTION_MOD].toInt();
  QString f_char = m_chatmessage[CHAR_NAME];
  QString f_custom_theme = ao_app->get_char_shouts(f_char);

  //if an objection is used
  if (objection_mod <= 4 && objection_mod >= 1)
  {
    switch (objection_mod)
    {
    case 1:
      ui_vp_objection->play("holdit_bubble", f_char, f_custom_theme, 724);
      objection_player->play("holdit.wav", f_char, f_custom_theme);
      break;
    case 2:
      ui_vp_objection->play("objection_bubble", f_char, f_custom_theme, 724);
      objection_player->play("objection.wav", f_char, f_custom_theme);
      if(ao_app->objection_stop_music())
          music_player->stop();
      break;
    case 3:
      ui_vp_objection->play("takethat_bubble", f_char, f_custom_theme, 724);
      objection_player->play("takethat.wav", f_char, f_custom_theme);
      break;
    //case 4 is AO2 only
    case 4:
      ui_vp_objection->play("custom", f_char, f_custom_theme, 724);
      objection_player->play("custom.wav", f_char, f_custom_theme);
      break;
    default:
      qDebug() << "W: Logic error in objection switch statement!";
    }
    sfx_player->clear(); //Objection played! Cut all sfx.
    int emote_mod = m_chatmessage[EMOTE_MOD].toInt();

    if (emote_mod == 0)
      m_chatmessage[EMOTE_MOD] = 1;
  }
  else
    handle_chatmessage_2();
}

void Courtroom::objection_done()
{
  handle_chatmessage_2();
}

void Courtroom::handle_chatmessage_2()
{
  ui_vp_speedlines->stop();
  ui_vp_player_char->stop();
  ui_vp_effect->stop();
  //Clear all looping sfx to prevent obnoxiousness
  sfx_player->loop_clear();

  if (!m_chatmessage[FRAME_SFX].isEmpty() && ao_app->is_frame_network_enabled())
  {
    //ORDER IS IMPORTANT!!
    QStringList netstrings = {m_chatmessage[FRAME_SCREENSHAKE], m_chatmessage[FRAME_REALIZATION], m_chatmessage[FRAME_SFX]};
    ui_vp_player_char->network_strings = netstrings;
  }
  else
    ui_vp_player_char->network_strings.clear();

  if (m_chatmessage[SHOWNAME].isEmpty() || !ui_showname_enable->isChecked())
  {
      QString real_name = char_list.at(m_chatmessage[CHAR_ID].toInt()).name;

      QString f_showname = ao_app->get_showname(real_name);

      ui_vp_showname->setText(f_showname);
  }
  else
  {
      ui_vp_showname->setText(m_chatmessage[SHOWNAME]);
  }

  if(ui_vp_showname->text().trimmed().isEmpty()) //Whitespace showname
  {
    ui_vp_chatbox->set_image("chatblank");
  }
  else //Aw yeah dude do some showname resizing magic
  {
    ui_vp_chatbox->set_image("chat");

    QFontMetrics fm(ui_vp_showname->font());
    int fm_width=fm.horizontalAdvance(ui_vp_showname->text());

    QString chatbox_path = ao_app->get_theme_path("chat");
    QString chatbox = ao_app->get_chat(m_chatmessage[CHAR_NAME]);
    if (chatbox != "")
    {
      chatbox_path = ao_app->get_base_path() + "misc/" + chatbox + "/chat";
    }

    pos_size_type default_width = ao_app->get_element_dimensions("showname", "courtroom_design.ini");
    int extra_width = ao_app->get_design_element("showname_extra_width", "courtroom_design.ini").toInt();

    if(extra_width > 0)
    {
      if (fm_width > default_width.width && ui_vp_chatbox->set_chatbox(chatbox_path + "med")) //This text be big. Let's do some shenanigans.
      {
        ui_vp_showname->resize(default_width.width+extra_width, ui_vp_showname->height());
        if (fm_width > ui_vp_showname->width() && ui_vp_chatbox->set_chatbox(chatbox_path + "big")) //Biggest possible size for us.
        {
          ui_vp_showname->resize(static_cast<int>(default_width.width+(extra_width*2)), ui_vp_showname->height());
        }
      }
      else
        ui_vp_showname->resize(default_width.width, ui_vp_showname->height());
    }
  }

  ui_vp_message->hide();
  ui_vp_chatbox->hide();

  QString design_file = "courtroom_fonts.ini";
  int f_weight = ao_app->get_font_size("message", design_file);
  QString font_name = ao_app->get_font_name("message_font", design_file);
  QColor f_color = ao_app->get_color("message_color", design_file);
  bool bold = ao_app->get_font_size("message_bold", design_file) == 1; // is the font bold or not?

  QString chatfont = ao_app->get_chat_font(m_chatmessage[CHAR_NAME]);
  if (chatfont != "")
    font_name = chatfont;

  int chatsize = ao_app->get_chat_size(m_chatmessage[CHAR_NAME]);
  if (chatsize != -1)
    f_weight = chatsize;
  this->set_qfont(ui_vp_message, "", QFont(font_name, f_weight), f_color, bold);

  set_scene(m_chatmessage[DESK_MOD], m_chatmessage[SIDE]);

  // Check if the message needs to be centered.
  QString f_message = m_chatmessage[MESSAGE];
  if (f_message.size() >= 2)
  {
      if (f_message.startsWith("~~"))
      {
          message_is_centered = true;
      }
      else
      {
          message_is_centered = false;
      }
  }
  else
  {
      ui_vp_message->setAlignment(Qt::AlignLeft);
  }


  int emote_mod = m_chatmessage[EMOTE_MOD].toInt();

  if (ao_app->flipping_enabled && m_chatmessage[FLIP].toInt() == 1)
    ui_vp_player_char->set_flipped(true);
  else
    ui_vp_player_char->set_flipped(false);

  QString side = m_chatmessage[SIDE];

  // Making the second character appear.
  if (m_chatmessage[OTHER_CHARID].isEmpty())
  {
    // If there is no second character, hide 'em, and center the first.
    ui_vp_sideplayer_char->stop();
    ui_vp_sideplayer_char->move(0,0);

    ui_vp_player_char->move(0,0);
  }
  else
  {
    bool ok;
    int got_other_charid = m_chatmessage[OTHER_CHARID].toInt(&ok);
    if (ok)
    {
      if (got_other_charid > -1)
      {
        // If there is, show them!
        ui_vp_sideplayer_char->show();

        // Depending on where we are, we offset the characters, and reorder their stacking.
        if (side == "def")
        {
          // We also move the character down depending on how far the are to the right.
          int hor_offset = m_chatmessage[SELF_OFFSET].toInt();
          int vert_offset = 0;
          if (hor_offset > 0)
          {
            vert_offset = hor_offset / 10;
          }
          ui_vp_player_char->move(ui_viewport->width() * hor_offset / 100, ui_viewport->height() * vert_offset / 100);

          // We do the same with the second character.
          int hor2_offset = m_chatmessage[OTHER_OFFSET].toInt();
          int vert2_offset = 0;
          if (hor2_offset > 0)
          {
            vert2_offset = hor2_offset / 10;
          }
          ui_vp_sideplayer_char->move(ui_viewport->width() * hor2_offset / 100, ui_viewport->height() * vert2_offset / 100);

          // Finally, we reorder them based on who is more to the left.
          // The person more to the left is more in the front.
          if (hor2_offset >= hor_offset)
          {
            ui_vp_sideplayer_char->stackUnder(ui_vp_player_char);
          }
          else
          {
            ui_vp_player_char->stackUnder(ui_vp_sideplayer_char);
          }
        }
        else if (side == "pro")
        {
          // Almost the same thing happens here, but in reverse.
          int hor_offset = m_chatmessage[SELF_OFFSET].toInt();
          int vert_offset = 0;
          if (hor_offset < 0)
          {
            // We don't want to RAISE the char off the floor.
            vert_offset = -1 * hor_offset / 10;
          }
          ui_vp_player_char->move(ui_viewport->width() * hor_offset / 100, ui_viewport->height() * vert_offset / 100);

          // We do the same with the second character.
          int hor2_offset = m_chatmessage[OTHER_OFFSET].toInt();
          int vert2_offset = 0;
          if (hor2_offset < 0)
          {
            vert2_offset = -1 * hor2_offset / 10;
          }
          ui_vp_sideplayer_char->move(ui_viewport->width() * hor2_offset / 100, ui_viewport->height() * vert2_offset / 100);

          // Finally, we reorder them based on who is more to the right.
          if (hor2_offset >= hor_offset)
          {
            ui_vp_sideplayer_char->stackUnder(ui_vp_player_char);
          }
          else
          {
            ui_vp_player_char->stackUnder(ui_vp_sideplayer_char);
          }
        }
        else
        {
          // In every other case, the person more to the left is on top.
          // These cases also don't move the characters down.
          int hor_offset = m_chatmessage[SELF_OFFSET].toInt();
          ui_vp_player_char->move(ui_viewport->width() * hor_offset / 100, 0);

          // We do the same with the second character.
          int hor2_offset = m_chatmessage[OTHER_OFFSET].toInt();
          ui_vp_sideplayer_char->move(ui_viewport->width() * hor2_offset / 100, 0);

          // Finally, we reorder them based on who is more to the left.
          // The person more to the left is more in the front.
          if (hor2_offset >= hor_offset)
          {
            ui_vp_sideplayer_char->stackUnder(ui_vp_player_char);
          }
          else
          {
            ui_vp_player_char->stackUnder(ui_vp_sideplayer_char);
          }
        }
        // We should probably also play the other character's idle emote.
        if (ao_app->flipping_enabled && m_chatmessage[OTHER_FLIP].toInt() == 1)
          ui_vp_sideplayer_char->set_flipped(true);
        else
          ui_vp_sideplayer_char->set_flipped(false);
        ui_vp_sideplayer_char->play_idle(m_chatmessage[OTHER_NAME], m_chatmessage[OTHER_EMOTE]);
      }
      else
      {
          // If the server understands other characters, but there
          // really is no second character, hide 'em, and center the first.
          ui_vp_sideplayer_char->hide();
          ui_vp_sideplayer_char->move(0,0);

          ui_vp_player_char->move(0,0);
      }
    }
  }
  switch (emote_mod)
  {
  case 1: case 2: case 6:
    play_preanim(false);
    break;
  case 0: case 5:
    if (m_chatmessage[NONINTERRUPTING_PRE].toInt() == 0)
      handle_chatmessage_3();
    else
      play_preanim(true);
    break;
  default:
    qDebug() << "W: invalid emote mod: " << QString::number(emote_mod);
  }
}

void Courtroom::do_screenshake()
{
  if(!ao_app->is_shake_flash_enabled())
      return;

  //This way, the animation is reset in such a way that last played screenshake would return to its "final frame" properly.
  //This properly resets all UI elements without having to bother keeping track of "origin" positions.
  //Works great wit the chat text being detached from the chat box!
  screenshake_animation_group->setCurrentTime(screenshake_animation_group->duration());
  screenshake_animation_group->clear();

  QList<QWidget *> affected_list = {
    ui_vp_background,
    ui_vp_player_char,
    ui_vp_sideplayer_char,
    ui_vp_chatbox
  };

  //I would prefer if this was its own "shake" function to be honest.
  foreach (QWidget* ui_element, affected_list)
  {
    QPropertyAnimation *screenshake_animation = new QPropertyAnimation(ui_element, "pos", this);
    QPoint pos_default = QPoint(ui_element->x(), ui_element->y());

    int duration = 300; //How long does the screenshake last
    int frequency = 20; //How often in ms is there a "jolt" frame
    int maxframes = duration/frequency;
    int max_x = 7; //Max deviation from origin on x axis
    int max_y = 7; //Max deviation from origin on y axis
    screenshake_animation->setDuration(duration);
    for (int frame=0; frame < maxframes; frame++)
    {
      double fraction = double(frame*frequency)/duration;
      quint32 rng = QRandomGenerator::global()->generate();
      int rand_x = int(rng) % max_x;
      int rand_y = int(rng+100) % max_y;
      screenshake_animation->setKeyValueAt(fraction, QPoint(pos_default.x() + rand_x, pos_default.y() + rand_y));
    }
    screenshake_animation->setEndValue(pos_default);
    screenshake_animation->setEasingCurve(QEasingCurve::Linear);
    screenshake_animation_group->addAnimation(screenshake_animation);
  }

  screenshake_animation_group->start();
}

void Courtroom::do_flash()
{
  if(!ao_app->is_shake_flash_enabled())
      return;

  QString f_char = m_chatmessage[CHAR_NAME];
  QString f_custom_theme = ao_app->get_char_shouts(f_char);
  ui_vp_effect->play("realizationflash", f_char, f_custom_theme, 60);
}

void Courtroom::do_effect(QString fx_name, QString fx_sound, QString p_char)
{
  if(!ao_app->is_shake_flash_enabled())
    return;

  QString effect = ao_app->get_effect(fx_name, p_char);
  if (effect == "")
    return;

  ui_vp_effect->set_play_once(false); // The effects themselves dictate whether or not they're looping. Static effects will linger.
  ui_vp_effect->play(effect); // It will set_play_once to true if the filepath provided is not designed to loop more than once
  if (fx_sound != "")
    sfx_player->play(ao_app->get_sfx_suffix(fx_sound));
}

void Courtroom::play_char_sfx(QString sfx_name)
{
  sfx_player->play(ao_app->get_sfx_suffix(sfx_name));
  if(ao_app->get_looping_sfx())
    sfx_player->set_looping(ao_app->get_sfx_looping(current_char, sfx_name)!="0");
}

void Courtroom::handle_chatmessage_3()
{
  start_chat_ticking();

  int f_evi_id = m_chatmessage[EVIDENCE_ID].toInt();
  QString f_side = m_chatmessage[SIDE];

  if (f_evi_id > 0 && f_evi_id <= local_evidence_list.size())
  {
    //shifted by 1 because 0 is no evidence per legacy standards
    QString f_image = local_evidence_list.at(f_evi_id - 1).image;
    //def jud and hlp should display the evidence icon on the RIGHT side
    bool is_left_side = !(f_side == "def" || f_side == "hlp" || f_side == "jud" || f_side == "jur");
    ui_vp_evidence_display->show_evidence(f_image, is_left_side, ui_sfx_slider->value());
  }

  int emote_mod = m_chatmessage[EMOTE_MOD].toInt();

  QString side = m_chatmessage[SIDE];

  if (emote_mod == 5 ||
      emote_mod == 6)
  {
    ui_vp_desk->hide();
    ui_vp_legacy_desk->hide();

    // Since we're zooming, hide the second character, and centre the first.
    ui_vp_sideplayer_char->hide();
    ui_vp_player_char->move(0,0);

    QString f_char = m_chatmessage[CHAR_NAME];
    QString f_custom_theme = ao_app->get_char_shouts(f_char);
    if (side == "pro" ||
        side == "hlp" ||
        side == "wit")
      ui_vp_speedlines->play("prosecution_speedlines", f_char, f_custom_theme);
    else
      ui_vp_speedlines->play("defense_speedlines", f_char, f_custom_theme);

  }

  //If this color is talking
  color_is_talking = color_markdown_talking_list.at(m_chatmessage[TEXT_COLOR].toInt());

  if (color_is_talking && text_state == 1 && anim_state < 2) //Set it to talking as we're not on that already
  {
    ui_vp_player_char->stop();
    ui_vp_player_char->play_talking(m_chatmessage[CHAR_NAME], m_chatmessage[EMOTE]);
    anim_state = 2;
  }
  else if (anim_state < 3) //Set it to idle as we're not on that already
  {
    ui_vp_player_char->stop();
    ui_vp_player_char->play_idle(m_chatmessage[CHAR_NAME], m_chatmessage[EMOTE]);
    anim_state = 3;
  }

  QString f_message = m_chatmessage[MESSAGE];
  QStringList call_words = ao_app->get_call_words();

  for (QString word : call_words)
  {
    if (f_message.contains(word, Qt::CaseInsensitive))
    {
      modcall_player->play(ao_app->get_sfx("word_call"));
      ao_app->alert(this);

      break;
    }
  }

}

QString Courtroom::filter_ic_text(QString p_text, bool html, int target_pos, int default_color)
{
  // Get rid of centering.
  if(p_text.startsWith("~~"))
      p_text.remove(0,2);

  p_text.remove("\n"); //Undesired newline chars, probably from copy-pasting it from a doc or something.

  QString p_text_escaped;

  int check_pos = 0;
  int check_pos_escaped = 0;
  bool ic_next_is_not_special = false;
  std::stack<int> ic_color_stack;

  if (html)
  {
    ic_color_stack.push(default_color);
    QString appendage = "<font color=\""+ color_rgb_list.at(default_color).name(QColor::HexRgb) +"\">";
    p_text_escaped.insert(check_pos_escaped, appendage);
    check_pos_escaped += appendage.size();
  }

  //Current issue: does not properly escape html stuff.
  //Solution: probably parse p_text and export into a different string separately, perform some mumbo jumbo to properly adjust string indexes.
  while (check_pos < p_text.size())
  {
    QString f_rest = p_text.right(p_text.size() - check_pos);
    QTextBoundaryFinder tbf(QTextBoundaryFinder::Grapheme, f_rest);
    QString f_character;
    int f_char_length;

    tbf.toNextBoundary();

    if (tbf.position() == -1)
      f_character = f_rest;
    else
      f_character = f_rest.left(tbf.position());

//    if (f_character == "&") //oh shit it's probably an escaped html
//    {
//      //Skip escaped chars like you would graphemes
//      QRegularExpression re("&([a-z0-9]+|#[0-9]{1,6}|#x[0-9a-f]{1,6});", QRegularExpression::CaseInsensitiveOption);
//      QRegularExpressionMatch match = re.match(f_rest);
//      if (match.hasMatch()) //OH SHIT IT IS, PANIC, PANIC
//      {
//        f_character = match.captured(0); //Phew, we solved the big problem here.
//      }
//    }

    f_character = f_character.toHtmlEscaped();

    if (f_character == " " && html) //Whitespace, woah
      f_character = "&nbsp;"; //Turn it into an HTML entity
    f_char_length = f_character.length();

    bool color_update = false;
    bool is_end = false;
    bool skip = false;

    if (!ic_next_is_not_special)
    {
      if (f_character == "\\")
      {
        ic_next_is_not_special = true;
        skip = true;
      }
      //Nothing related to colors here
      else if (f_character == "{" || f_character == "}" || f_character == "@" || f_character == "$")
      {
        skip = true;
      }
      //Parse markdown colors
      else
      {
        for (int c = 0; c < max_colors; ++c)
        {
          //Clear the stored optimization information
          QString markdown_start = color_markdown_start_list.at(c).toHtmlEscaped();
          QString markdown_end = color_markdown_end_list.at(c).toHtmlEscaped();
          bool markdown_remove = color_markdown_remove_list.at(c);
          if (markdown_start.isEmpty()) //Not defined
            continue;

          if (markdown_end.isEmpty() || markdown_end == markdown_start) //"toggle switch" type
          {
            if (f_character == markdown_start)
            {
              if (html)
              {
                if (!ic_color_stack.empty() && ic_color_stack.top() == c && default_color != c)
                {
                  ic_color_stack.pop(); //Cease our coloring
                  is_end = true;
                }
                else
                {
                  ic_color_stack.push(c); //Begin our coloring
                }
                color_update = true;
              }
              skip = markdown_remove;
              break; //Prevent it from looping forward for whatever reason
            }
          }
          else if (f_character == markdown_start || (f_character == markdown_end && !ic_color_stack.empty() && ic_color_stack.top() == c))
          {
            if (html)
            {
              if (f_character == markdown_end)
              {
                ic_color_stack.pop(); //Cease our coloring
                is_end = true;
              }
              else if (f_character == markdown_start)
              {
                ic_color_stack.push(c); //Begin our coloring
              }
              color_update = true;
            }
            skip = markdown_remove;
            break; //Prevent it from looping forward for whatever reason
          }
        }
        //Parse the newest color stack
        if (color_update && (target_pos <= -1 || check_pos < target_pos))
        {
          if (!ic_next_is_not_special)
          {
            QString appendage = "</font>";

            if (!ic_color_stack.empty())
              appendage += "<font color=\""+ color_rgb_list.at(ic_color_stack.top()).name(QColor::HexRgb) +"\">";

            if (is_end && !skip)
            {
              p_text_escaped.insert(check_pos_escaped, f_character); //Add that char right now
              check_pos_escaped += f_char_length; //So the closing char is captured too
              skip = true;
            }
            p_text_escaped.insert(check_pos_escaped, appendage);
            check_pos_escaped += appendage.size();
          }
        }
      }
    }
    else
    {
      if (f_character == "n") // \n, that's a line break son
      {
        QString appendage = "<br/>";
        if (!html)
        {
          //actual newline commented out
//          appendage = "\n";
//          size = 1; //yeah guess what \n is a "single character" apparently
          appendage = "\\n "; //visual representation of a newline
        }
        p_text_escaped.insert(check_pos_escaped, appendage);
        check_pos_escaped += appendage.size();
        skip = true;
      }

      ic_next_is_not_special = false;
    }

    //Make all chars we're not supposed to see invisible
    if (target_pos > -1 && check_pos == target_pos)
    {
      QString appendage = "";
      if (!ic_color_stack.empty())
      {
        if (!is_end) //Was our last coloring char ending the color stack or nah
        {
          //God forgive me for my transgressions but I have refactored this whole thing about 25 times and having to refactor it
          //again to more elegantly support this will finally make me go insane.
          color_is_talking = color_markdown_talking_list.at(ic_color_stack.top());
        }

        //Clean it up, we're done here
        while (!ic_color_stack.empty())
            ic_color_stack.pop();

        appendage += "</font>";
      }
      ic_color_stack.push(-1); //Dummy colorstack push for maximum </font> appendage
      appendage += "<font color=\"#00000000\">";
      p_text_escaped.insert(check_pos_escaped, appendage);
      check_pos_escaped += appendage.size();
    }
    if (!skip)
    {
      p_text_escaped.insert(check_pos_escaped, f_character);
      check_pos_escaped += f_char_length;
    }
    check_pos += 1;
  }

  if (!ic_color_stack.empty())
  {
    p_text_escaped.append("</font>");
  }

  return p_text_escaped;
}

void Courtroom::append_ic_text(QString p_text, QString p_name, bool is_songchange)
{
  QTextCharFormat bold;
  QTextCharFormat normal;
  QTextCharFormat italics;
  bold.setFontWeight(QFont::Bold);
  normal.setFontWeight(QFont::Normal);
  italics.setFontItalic(true);
  const QTextCursor old_cursor = ui_ic_chatlog->textCursor();
  const int old_scrollbar_value = ui_ic_chatlog->verticalScrollBar()->value();

  if (!is_songchange)
    p_text = filter_ic_text(p_text, false);

  if (log_goes_downwards)
  {
      const bool is_scrolled_down = old_scrollbar_value == ui_ic_chatlog->verticalScrollBar()->maximum();

      ui_ic_chatlog->moveCursor(QTextCursor::End);

      if (!first_message_sent)
      {
          ui_ic_chatlog->textCursor().insertText(p_name, bold);
          first_message_sent = true;
      }
      else
      {
          ui_ic_chatlog->textCursor().insertText('\n' + p_name, bold);
      }

      if (is_songchange)
      {
        ui_ic_chatlog->textCursor().insertText(" has played a song: ", normal);
        ui_ic_chatlog->textCursor().insertText(p_text + ".", italics);
      }
      else
      {
        ui_ic_chatlog->textCursor().insertText(": ", normal);
        ui_ic_chatlog->textCursor().insertText(p_text, normal);
      }

      // If we got too many blocks in the current log, delete some from the top.
      while (ui_ic_chatlog->document()->blockCount() > log_maximum_blocks && log_maximum_blocks > 0)
      {
          ui_ic_chatlog->moveCursor(QTextCursor::Start);
          ui_ic_chatlog->textCursor().select(QTextCursor::BlockUnderCursor);
          ui_ic_chatlog->textCursor().removeSelectedText();
          ui_ic_chatlog->textCursor().deleteChar();
      }

      if (old_cursor.hasSelection() || !is_scrolled_down)
      {
          // The user has selected text or scrolled away from the bottom: maintain position.
          ui_ic_chatlog->setTextCursor(old_cursor);
          ui_ic_chatlog->verticalScrollBar()->setValue(old_scrollbar_value);
      }
      else
      {
          // The user hasn't selected any text and the scrollbar is at the bottom: scroll to the bottom.
          ui_ic_chatlog->moveCursor(QTextCursor::End);
          ui_ic_chatlog->verticalScrollBar()->setValue(ui_ic_chatlog->verticalScrollBar()->maximum());
      }
  }
  else
  {
      const bool is_scrolled_up = old_scrollbar_value == ui_ic_chatlog->verticalScrollBar()->minimum();

      ui_ic_chatlog->moveCursor(QTextCursor::Start);

      ui_ic_chatlog->textCursor().insertText(p_name, bold);

      if (is_songchange)
      {
        ui_ic_chatlog->textCursor().insertText(" has played a song: ", normal);
        ui_ic_chatlog->textCursor().insertText(p_text + "." + '\n', italics);
      }
      else
      {
        ui_ic_chatlog->textCursor().insertText(": ", normal);
        ui_ic_chatlog->textCursor().insertText(p_text + '\n', normal);
      }

      // If we got too many blocks in the current log, delete some from the bottom.
      while (ui_ic_chatlog->document()->blockCount() > log_maximum_blocks && log_maximum_blocks > 0)
      {
          ui_ic_chatlog->moveCursor(QTextCursor::End);
          ui_ic_chatlog->textCursor().select(QTextCursor::BlockUnderCursor);
          ui_ic_chatlog->textCursor().removeSelectedText();
          ui_ic_chatlog->textCursor().deletePreviousChar();
      }

      if (old_cursor.hasSelection() || !is_scrolled_up)
      {
          // The user has selected text or scrolled away from the top: maintain position.
          ui_ic_chatlog->setTextCursor(old_cursor);
          ui_ic_chatlog->verticalScrollBar()->setValue(old_scrollbar_value);
      }
      else
      {
          // The user hasn't selected any text and the scrollbar is at the top: scroll to the top.
          ui_ic_chatlog->moveCursor(QTextCursor::Start);
          ui_ic_chatlog->verticalScrollBar()->setValue(ui_ic_chatlog->verticalScrollBar()->minimum());
      }
  }
}

void Courtroom::play_preanim(bool noninterrupting)
{
  QString f_char = m_chatmessage[CHAR_NAME];
  QString f_preanim = m_chatmessage[PRE_EMOTE];

  //all time values in char.inis are multiplied by a constant(time_mod) to get the actual time
  int ao2_duration = ao_app->get_ao2_preanim_duration(f_char, f_preanim);
  int text_delay = ao_app->get_text_delay(f_char, f_preanim) * time_mod;
  int sfx_delay = m_chatmessage[SFX_DELAY].toInt() * 60;

  int preanim_duration;

  if (ao2_duration < 0)
    preanim_duration = ao_app->get_preanim_duration(f_char, f_preanim);
  else
    preanim_duration = ao2_duration;

  sfx_delay_timer->start(sfx_delay);
  QString anim_to_find = ao_app->get_image_suffix(ao_app->get_character_path(f_char, f_preanim));
  if (!file_exists(anim_to_find))
  {
    if (noninterrupting)
      anim_state = 4;
    else
      anim_state = 1;
    preanim_done();
    qDebug() << "could not find " + anim_to_find;
    return;
  }

  ui_vp_player_char->play_pre(f_char, f_preanim, preanim_duration);

  if (noninterrupting)
    anim_state = 4;
  else
    anim_state = 1;

  if (text_delay >= 0)
    text_delay_timer->start(text_delay);

  if (noninterrupting)
    handle_chatmessage_3();
}

void Courtroom::preanim_done()
{
  anim_state = 1;
  handle_chatmessage_3();
}


void Courtroom::start_chat_ticking()
{
  //we need to ensure that the text isn't already ticking because this function can be called by two logic paths
  if (text_state != 0)
    return;

  if (m_chatmessage[EFFECTS] != "")
  {
    QStringList fx_list = m_chatmessage[EFFECTS].split("|");
    QString fx = fx_list[0];
    QString fx_sound;
    if (fx_list.length() > 1)
      fx_sound = fx_list[1];

    this->do_effect(fx, fx_sound, m_chatmessage[CHAR_NAME]);
  }
  else if (m_chatmessage[REALIZATION] == "1")
  {
    this->do_flash();
    sfx_player->play(ao_app->get_custom_realization(m_chatmessage[CHAR_NAME]));
  }

  if (m_chatmessage[SCREENSHAKE] == "1")
  {
    this->do_screenshake();
  }

  if (chatmessage_is_empty)
  {
    //since the message is empty, it's technically done ticking
    text_state = 2;
    return;
  }

  ui_vp_chatbox->show();
  ui_vp_message->show();

  if (!is_additive)
  {
    ui_vp_message->clear();
    real_tick_pos = 0;
    additive_previous = "";
  }

  tick_pos = 0;
  blip_ticker = 0;

  // At the start of every new message, we set the text speed to the default.
  current_display_speed = 3;
  chat_tick_timer->start(0); //Display the first char right away

  QString f_gender = ao_app->get_gender(m_chatmessage[CHAR_NAME]);

  blip_player->set_blips(ao_app->get_sfx_suffix("sfx-blip" + f_gender));

  //means text is currently ticking
  text_state = 1;
}

void Courtroom::chat_tick()
{
  //note: this is called fairly often
  //do not perform heavy operations here

  QString f_message = m_chatmessage[MESSAGE];

  // Due to our new text speed system, we always need to stop the timer now.
  chat_tick_timer->stop();

  if (tick_pos >= f_message.size())
  {
    text_state = 2;
    if (anim_state < 3)
    {
      anim_state = 3;
      ui_vp_player_char->play_idle(m_chatmessage[CHAR_NAME], m_chatmessage[EMOTE]);
    }
    QString f_char = m_chatmessage[CHAR_NAME];
    QString f_custom_theme = ao_app->get_char_shouts(f_char);
    ui_vp_chat_arrow->play("chat_arrow", f_char, f_custom_theme); //Chat stopped being processed, indicate that.
    additive_previous = additive_previous + filter_ic_text(f_message, true, -1, m_chatmessage[TEXT_COLOR].toInt());
    real_tick_pos = ui_vp_message->toPlainText().size();

    QScrollBar *scroll = ui_vp_message->verticalScrollBar();
    scroll->setValue(scroll->maximum());
    return;
  }

  // Stops blips from playing when we have a formatting option.
  bool formatting_char = false;

  QString f_rest = f_message;
  f_rest.remove(0, tick_pos);
  QTextBoundaryFinder tbf(QTextBoundaryFinder::Grapheme, f_rest);
  QString f_character;
  int f_char_length;

  tbf.toNextBoundary();

  if (tbf.position() == -1)
    f_character = f_rest;
  else
    f_character = f_rest.left(tbf.position());

  f_char_length = f_character.length();

  // Escape character.
  if (!next_character_is_not_special)
  {
    if (f_character == "\\")
    {
        next_character_is_not_special = true;
        formatting_char = true;
    }

    // Text speed modifier.
    else if (f_character == "{")
    {
        // ++, because it INCREASES delay!
        current_display_speed++;
        formatting_char = true;
    }
    else if (f_character == "}")
    {
        current_display_speed--;
        formatting_char = true;
    }

    //Screenshake.
    else if (f_character == "@")
    {
        this->do_screenshake();
        formatting_char = true;
    }

    //Flash.
    else if (f_character == "$")
    {
        this->do_flash();
        formatting_char = true;
    }
    else
    {
      //Parse markdown colors
      for (int c = 0; c < max_colors; ++c)
      {
        QString markdown_start = color_markdown_start_list.at(c);
        QString markdown_end = color_markdown_end_list.at(c);
        bool markdown_remove = color_markdown_remove_list.at(c);
        if (markdown_start.isEmpty())
          continue;

        if (f_character == markdown_start || f_character == markdown_end)
        {
          if (markdown_remove)
            formatting_char = true;
          break;
        }
      }
    }
  }
  else
  {
    if (f_character == "n")
      formatting_char = true; //it's a newline
    next_character_is_not_special = false;
  }

  tick_pos += f_char_length;

  //Do the colors, gradual showing, etc. in here
  ui_vp_message->setHtml(additive_previous + filter_ic_text(f_message, true, tick_pos, m_chatmessage[TEXT_COLOR].toInt()));

  if (!formatting_char || f_character == "n") //NEWLINES (\n) COUNT AS A SINGLE CHARACTER.
  {
    //Make the cursor follow the message
    QTextCursor cursor = ui_vp_message->textCursor();
    cursor.setPosition(real_tick_pos);
    ui_vp_message->setTextCursor(cursor);
    real_tick_pos += f_char_length;
  }
  ui_vp_message->ensureCursorVisible();

//  //Grab the currently displayed chars
//  f_rest = f_message.left(tick_pos);
//  f_rest.replace("\\n", "\n");

//  QFontMetrics fm = fontMetrics();
//  QRect bounding_rect = fm.boundingRect(QRect(0,0,ui_vp_message->width(),ui_vp_message->height()), Qt::TextWordWrap, f_rest);

//  //If the text overflows, make it snap to bottom
//  if (bounding_rect.height() > ui_vp_message->height())
//  {

//    QScrollBar *scroll = ui_vp_message->verticalScrollBar();
//    scroll->value();
//    scroll->setValue(scroll->maximum());
//  }

  // Keep the speed at bay.
  if (current_display_speed < 0)
    current_display_speed = 0;
  else if (current_display_speed > 6)
    current_display_speed = 6;

  //Blip player and real tick pos ticker
  if (!formatting_char && (f_character != ' ' || blank_blip))
  {
    if (blip_ticker % blip_rate == 0)
    {
      blip_player->blip_tick();
    }
    ++blip_ticker;
  }

  // If we had a formatting char, we shouldn't wait so long again, as it won't appear!
  // Additionally, if the message_display_speed length is too short for us to do anything (play animations, etc.) then skip the trouble and don't bother.
  if (formatting_char || message_display_speed[current_display_speed] <= 0)
  {
     chat_tick_timer->start(0);
  }
  else
  {
    //If this color is talking
    if (color_is_talking && anim_state != 2 && anim_state < 4) //Set it to talking as we're not on that already (though we have to avoid interrupting a non-interrupted preanim)
    {
      ui_vp_player_char->stop();
      ui_vp_player_char->play_talking(m_chatmessage[CHAR_NAME], m_chatmessage[EMOTE]);
      anim_state = 2;
    }
    else if (!color_is_talking && anim_state < 3 && anim_state != 3) //Set it to idle as we're not on that already
    {
      ui_vp_player_char->stop();
      ui_vp_player_char->play_idle(m_chatmessage[CHAR_NAME], m_chatmessage[EMOTE]);
      anim_state = 3;
    }

    //Continue ticking
    chat_tick_timer->start(message_display_speed[current_display_speed]);
  }
}

void Courtroom::play_sfx()
{
  QString sfx_name = m_chatmessage[SFX_NAME];

  if (sfx_name == "1")
    return;

  sfx_player->play(ao_app->get_sfx_suffix(sfx_name));
  if(ao_app->get_looping_sfx())
    sfx_player->set_looping(ao_app->get_sfx_looping(current_char, sfx_name)!="0");
}

void Courtroom::set_scene(QString f_desk_mod, QString f_side)
{
  //witness is default if pos is invalid
  QString f_background = "witnessempty";
  QString f_desk_image = "stand";

  if (f_side == "def")
  {
    f_background = "defenseempty";
    if (is_ao2_bg)
      f_desk_image = "defensedesk";
    else
      f_desk_image = "bancodefensa";
  }
  else if (f_side == "pro")
  {
    f_background = "prosecutorempty";
    if (is_ao2_bg)
      f_desk_image = "prosecutiondesk";
    else
      f_desk_image = "bancoacusacion";
  }
  else if (f_side == "jud")
  {
    f_background = "judgestand";
    f_desk_image = "judgedesk";
  }
  else if (f_side == "hld")
  {
    f_background = "helperstand";
    f_desk_image = "helperdesk";
  }
  else if (f_side == "hlp")
  {
    f_background = "prohelperstand";
    f_desk_image = "prohelperdesk";
  }
  else if (f_side == "jur" && file_exists(ao_app->get_image_suffix(ao_app->get_background_path("jurystand"))))
  {
    f_background = "jurystand";
    f_desk_image = "jurydesk";
  }
  else if (f_side == "sea" && file_exists(ao_app->get_image_suffix(ao_app->get_background_path("seancestand"))))
  {
    f_background = "seancestand";
    f_desk_image = "seancedesk";
  }
  else
  {
    if (is_ao2_bg)
      f_desk_image = "stand";
    else
      f_desk_image = "estrado";
  }

  ui_vp_background->set_image(f_background);
  ui_vp_desk->set_image(f_desk_image);
  ui_vp_legacy_desk->set_legacy_desk(f_desk_image);

  if (f_desk_mod == "0" || (f_desk_mod != "1" &&
           (f_side == "jud" ||
            f_side == "hld" ||
            f_side == "hlp")))
  {
    ui_vp_desk->hide();
    ui_vp_legacy_desk->hide();
  }
  else if (is_ao2_bg || (f_side == "jud" ||
                         f_side == "hld" ||
                         f_side == "hlp"))
  {
    ui_vp_legacy_desk->hide();
    ui_vp_desk->show();
  }
  else
  {
    if (f_side == "wit")
    {
      ui_vp_desk->show();
      ui_vp_legacy_desk->hide();
    }
    else
    {
      ui_vp_desk->hide();
      ui_vp_legacy_desk->show();
    }
  }
}

void Courtroom::set_ip_list(QString p_list)
{
  QString f_list = p_list.replace("|", ":").replace("*", "\n");

  ui_server_chatlog->append(f_list);
}

void Courtroom::set_mute(bool p_muted, int p_cid)
{
  if (p_cid != m_cid && p_cid != -1)
    return;

  if (p_muted)
    ui_muted->show();
  else
  {
    ui_muted->hide();
    ui_ic_chat_message->setFocus();
  }

  ui_muted->resize(ui_ic_chat_message->width(), ui_ic_chat_message->height());
  ui_muted->set_image("muted");

  is_muted = p_muted;
  ui_ic_chat_message->setEnabled(!p_muted);
}

void Courtroom::set_ban(int p_cid)
{
  if (p_cid != m_cid && p_cid != -1)
    return;

  call_notice("You have been banned.");

  ao_app->construct_lobby();
  ao_app->destruct_courtroom();
}

void Courtroom::handle_song(QStringList *p_contents)
{
  QStringList f_contents = *p_contents;

  if (f_contents.size() < 2)
    return;

  QString f_song = f_contents.at(0);
  QString f_song_clear = f_song;
  f_song_clear = f_song_clear.left(f_song_clear.lastIndexOf("."));
  int n_char = f_contents.at(1).toInt();

  bool looping = true;
  int channel = 0;
  bool crossfade = false;
  if (n_char < 0 || n_char >= char_list.size())
  {
    int channel = 0;
    if (p_contents->length() > 3 && p_contents->at(3) != "-1")
      looping = false;

    if (p_contents->length() > 4) //eyyy we want to change this song's CHANNEL huh
      channel = p_contents->at(4).toInt(); //let the music player handle it if it's bigger than the channel list

    if (p_contents->length() > 5) //CROSSFADE!? Are you MAD?
    {
      crossfade = p_contents->at(5) == "1"; //let the music player handle it if it's bigger than the channel list
    }

    music_player->set_looping(looping, channel);
    music_player->play(f_song, channel, crossfade);
    if (channel == 0)
      ui_music_name->setText(f_song);
  }
  else
  {
    QString str_char = char_list.at(n_char).name;
    QString str_show = char_list.at(n_char).name;

    if (p_contents->length() > 2)
    {
        if(p_contents->at(2) != "")
        {
          str_show = p_contents->at(2);
        }
    }
    if (p_contents->length() > 3 && p_contents->at(3) != "-1")
    {
        //I am really confused why "-1" is "loop this song" and why anything else passes as "don't loop"
        //(if we even have this length) but alright
        looping = false;
    }
    if (p_contents->length() > 4) //eyyy we want to change this song's CHANNEL huh
      channel = p_contents->at(4).toInt(); //let the music player handle it if it's bigger than the channel list

    if (p_contents->length() > 5) //CROSSFADE!? Are you MAD?
      crossfade = p_contents->at(5) == "1"; //let the music player handle it if it's bigger than the channel list

    if (!mute_map.value(n_char))
    {
      chatlogpiece* temp = new chatlogpiece(str_char, str_show, f_song, true);
      ic_chatlog_history.append(*temp);
      ao_app->append_to_file(temp->get_full(), ao_app->log_filename, true);

      while(ic_chatlog_history.size() > log_maximum_blocks && log_maximum_blocks > 0)
      {
        ic_chatlog_history.removeFirst();
      }

      append_ic_text(f_song_clear, str_show, true);
      music_player->set_looping(looping, channel);
      music_player->play(f_song, channel, crossfade);
      if (channel == 0)
        ui_music_name->setText(f_song);
    }
  }
}

void Courtroom::handle_wtce(QString p_wtce, int variant)
{
  QString sfx_file = "courtroom_sounds.ini";

  //witness testimony
  if (p_wtce == "testimony1")
  {
    sfx_player->play(ao_app->get_sfx("witness_testimony"));
    ui_vp_wtce->play("witnesstestimony", "", "", 1500);
    ui_vp_testimony->play("testimony");
  }
  //cross examination
  else if (p_wtce == "testimony2")
  {
    sfx_player->play(ao_app->get_sfx("cross_examination"));
    ui_vp_wtce->play("crossexamination", "", "", 1500);
    ui_vp_testimony->stop();
  }
  else if (p_wtce == "judgeruling")
  {
    if (variant == 0)
    {
        sfx_player->play(ao_app->get_sfx("not_guilty"));
        ui_vp_wtce->play("notguilty", "", "", 3000);
        ui_vp_testimony->stop();
    }
    else if (variant == 1) {
        sfx_player->play(ao_app->get_sfx("guilty"));
        ui_vp_wtce->play("guilty", "", "", 3000);
        ui_vp_testimony->stop();
    }
  }
}

void Courtroom::set_hp_bar(int p_bar, int p_state)
{
  if (p_state < 0 || p_state > 10)
    return;

  if (p_bar == 1)
  {
    ui_defense_bar->set_image("defensebar" + QString::number(p_state));
    defense_bar_state = p_state;
  }
  else if (p_bar == 2)
  {
    ui_prosecution_bar->set_image("prosecutionbar" + QString::number(p_state));
    prosecution_bar_state = p_state;
  }
}

void Courtroom::toggle_judge_buttons(bool is_on)
{
  if (is_on)
  {
    ui_witness_testimony->show();
    ui_cross_examination->show();
    ui_guilty->show();
    ui_not_guilty->show();
    ui_defense_minus->show();
    ui_defense_plus->show();
    ui_prosecution_minus->show();
    ui_prosecution_plus->show();
  }
  else
  {
    ui_witness_testimony->hide();
    ui_cross_examination->hide();
    ui_guilty->hide();
    ui_not_guilty->hide();
    ui_defense_minus->hide();
    ui_defense_plus->hide();
    ui_prosecution_minus->hide();
    ui_prosecution_plus->hide();
  }
}

void Courtroom::mod_called(QString p_ip)
{
  ui_server_chatlog->append(p_ip);
  if (!ui_guard->isChecked())
  {
    modcall_player->play(ao_app->get_sfx("mod_call"));
    ao_app->alert(this);
  }
}

void Courtroom::case_called(QString msg, bool def, bool pro, bool jud, bool jur, bool steno)
{
  if (ui_casing->isChecked())
  {
    ui_server_chatlog->append(msg);
    if ((ao_app->get_casing_defence_enabled() && def) ||
        (ao_app->get_casing_prosecution_enabled() && pro) ||
        (ao_app->get_casing_judge_enabled() && jud) ||
        (ao_app->get_casing_juror_enabled() && jur) ||
        (ao_app->get_casing_steno_enabled() && steno))
    {
        modcall_player->play(ao_app->get_sfx("case_call"));
        ao_app->alert(this);
    }
  }
}

void Courtroom::on_ooc_return_pressed()
{
  QString ooc_message = ui_ooc_chat_message->text();

  if (ooc_message == "" || ui_ooc_chat_name->text() == "")
    return;

  if (ooc_message.startsWith("/pos"))
  {
    if (ooc_message == "/pos jud")
    {
      toggle_judge_buttons(true);
    }
    else
    {
      toggle_judge_buttons(false);
    }
  }
  else if (ooc_message.startsWith("/rainbow") && ao_app->yellow_text_enabled && !rainbow_appended)
  {
    //ui_text_color->addItem("Rainbow");
    ui_ooc_chat_message->clear();
    //rainbow_appended = true;
    append_server_chatmessage("CLIENT", tr("This does nothing, but there you go."), "1");
    return;
  }
  else if (ooc_message.startsWith("/settings"))
  {
    ui_ooc_chat_message->clear();
    ao_app->call_settings_menu();
    append_server_chatmessage("CLIENT", tr("You opened the settings menu."), "1");
    return;
  }
  else if (ooc_message.startsWith("/pair"))
  {
    ui_ooc_chat_message->clear();
    ooc_message.remove(0,6);

    bool ok;
    int whom = ooc_message.toInt(&ok);
    if (ok)
    {
      if (whom > -1)
      {
        other_charid = whom;
        QString msg = tr("You will now pair up with ");
        msg.append(char_list.at(whom).name);
        msg.append(tr(" if they also choose your character in return."));
        append_server_chatmessage("CLIENT", msg, "1");
      }
      else
      {
        other_charid = -1;
        append_server_chatmessage("CLIENT", tr("You are no longer paired with anyone."), "1");
      }
    }
    else
    {
      append_server_chatmessage("CLIENT", tr("Are you sure you typed that well? The char ID could not be recognised."), "1");
    }
    return;
  }
  else if (ooc_message.startsWith("/offset"))
  {
    ui_ooc_chat_message->clear();
    ooc_message.remove(0,8);

    bool ok;
    int off = ooc_message.toInt(&ok);
    if (ok)
    {
      if (off >= -100 && off <= 100)
      {
        offset_with_pair = off;
        QString msg = tr("You have set your offset to ");
        msg.append(QString::number(off));
        msg.append("%.");
        append_server_chatmessage("CLIENT", msg, "1");
      }
      else
      {
        append_server_chatmessage("CLIENT", tr("Your offset must be between -100% and 100%!"), "1");
      }
    }
    else
    {
      append_server_chatmessage("CLIENT", tr("That offset does not look like one."), "1");
    }
    return;
  }
  else if (ooc_message.startsWith("/switch_am"))
  {
      append_server_chatmessage("CLIENT", tr("You switched your music and area list."), "1");
      on_switch_area_music_clicked();
      ui_ooc_chat_message->clear();
      return;
  }
  else if (ooc_message.startsWith("/enable_blocks"))
  {
    append_server_chatmessage("CLIENT", tr("You have forcefully enabled features that the server may not support. You may not be able to talk IC, or worse, because of this."), "1");
    ao_app->cccc_ic_support_enabled = true;
    ao_app->arup_enabled = true;
    ao_app->modcall_reason_enabled = true;
    on_reload_theme_clicked();
    ui_ooc_chat_message->clear();
    return;
  }
  else if (ooc_message.startsWith("/non_int_pre"))
  {
    if (ui_pre_non_interrupt->isChecked())
      append_server_chatmessage("CLIENT", tr("Your pre-animations interrupt again."), "1");
    else
      append_server_chatmessage("CLIENT", tr("Your pre-animations will not interrupt text."), "1");
    ui_pre_non_interrupt->setChecked(!ui_pre_non_interrupt->isChecked());
    ui_ooc_chat_message->clear();
    return;
  }
  else if (ooc_message.startsWith("/save_chatlog"))
  {
    QFile file("chatlog.txt");

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
      append_server_chatmessage("CLIENT", tr("Couldn't open chatlog.txt to write into."), "1");
      ui_ooc_chat_message->clear();
      return;
    }

    QTextStream out(&file);

    foreach (chatlogpiece item, ic_chatlog_history) {
        out << item.get_full() << '\n';
      }

    file.close();

    append_server_chatmessage("CLIENT", tr("The IC chatlog has been saved."), "1");
    ui_ooc_chat_message->clear();
    return;
  }
  else if (ooc_message.startsWith("/load_case"))
  {
    QStringList command = ooc_message.split(" ", QString::SkipEmptyParts);

    QDir casefolder("base/cases");
    if (!casefolder.exists())
    {
        QDir::current().mkdir("base/" + casefolder.dirName());
        append_server_chatmessage("CLIENT", tr("You don't have a `base/cases/` folder! It was just made for you, but seeing as it WAS just made for you, it's likely the case file you're looking for can't be found in there."), "1");
        ui_ooc_chat_message->clear();
        return;
    }
    QStringList caseslist = casefolder.entryList();
    caseslist.removeOne(".");
    caseslist.removeOne("..");
    caseslist.replaceInStrings(".ini","");

    if (command.size() < 2)
    {
      append_server_chatmessage("CLIENT", tr("You need to give a filename to load (extension not needed)! Make sure that it is in the `base/cases/` folder, and that it is a correctly formatted ini.\nCases you can load: %1").arg(caseslist.join(", ")), "1");
      ui_ooc_chat_message->clear();
      return;
    }


    if (command.size() > 2)
    {
      append_server_chatmessage("CLIENT", tr("Too many arguments to load a case! You only need one filename, without extension."), "1");
      ui_ooc_chat_message->clear();
      return;
    }

    QSettings casefile("base/cases/" + command[1] + ".ini", QSettings::IniFormat);

    QString caseauth = casefile.value("author", "").value<QString>();
    QString casedoc = casefile.value("doc", "").value<QString>();
    QString cmdoc = casefile.value("cmdoc", "").value<QString>();
    QString casestatus = casefile.value("status", "").value<QString>();

    if (!caseauth.isEmpty())
      append_server_chatmessage("CLIENT", tr("Case made by %1.").arg(caseauth), "1");
    if (!casedoc.isEmpty())
      ao_app->send_server_packet(new AOPacket("CT#" + ui_ooc_chat_name->text() + "#/doc " + casedoc + "#%"));
    if (!casestatus.isEmpty())
      ao_app->send_server_packet(new AOPacket("CT#" + ui_ooc_chat_name->text() + "#/status " + casestatus + "#%"));
    if (!cmdoc.isEmpty())
      append_server_chatmessage("CLIENT", tr("Navigate to %1 for the CM doc.").arg(cmdoc), "1");

    for (int i = local_evidence_list.size() - 1; i >= 0; i--) {
        ao_app->send_server_packet(new AOPacket("DE#" + QString::number(i) + "#%"));
    }

    foreach (QString evi, casefile.childGroups()) {
        if (evi == "General")
          continue;

        QStringList f_contents;

        f_contents.append(casefile.value(evi + "/name", "UNKNOWN").value<QString>());
        f_contents.append(casefile.value(evi + "/description", "UNKNOWN").value<QString>());
        f_contents.append(casefile.value(evi + "/image", "UNKNOWN.png").value<QString>());

        ao_app->send_server_packet(new AOPacket("PE", f_contents));
      }

    append_server_chatmessage("CLIENT", tr("Your case \"%1\" was loaded!").arg(command[1]), "1");
    ui_ooc_chat_message->clear();
    return;
  }
  else if(ooc_message.startsWith("/save_case"))
  {
      QStringList command = ooc_message.split(" ", QString::SkipEmptyParts);

      QDir casefolder("base/cases");
      if (!casefolder.exists())
      {
          QDir::current().mkdir("base/" + casefolder.dirName());
          append_server_chatmessage("CLIENT", tr("You don't have a `base/cases/` folder! It was just made for you, but seeing as it WAS just made for you, it's likely that you somehow deleted it."), "1");
          ui_ooc_chat_message->clear();
          return;
      }
      QStringList caseslist = casefolder.entryList();
      caseslist.removeOne(".");
      caseslist.removeOne("..");
      caseslist.replaceInStrings(".ini","");

      if (command.size() < 3)
      {
        append_server_chatmessage("CLIENT", tr("You need to give a filename to save (extension not needed) and the courtroom status!"), "1");
        ui_ooc_chat_message->clear();
        return;
      }


      if (command.size() > 3)
      {
        append_server_chatmessage("CLIENT", tr("Too many arguments to save a case! You only need a filename without extension and the courtroom status!"), "1");
        ui_ooc_chat_message->clear();
        return;
      }
      QSettings casefile("base/cases/" + command[1] + ".ini", QSettings::IniFormat);
      casefile.setValue("author",ui_ooc_chat_name->text());
      casefile.setValue("cmdoc","");
      casefile.setValue("doc", "");
      casefile.setValue("status",command[2]);
      casefile.sync();
      for(int i = local_evidence_list.size() - 1; i >= 0; i--)
      {
           QString clean_evidence_dsc =  local_evidence_list[i].description.replace(QRegularExpression("<owner = ...>..."), "");
           clean_evidence_dsc = clean_evidence_dsc.replace(clean_evidence_dsc.lastIndexOf(">"), 1, "");
           casefile.beginGroup(QString::number(i));
           casefile.sync();
           casefile.setValue("name",local_evidence_list[i].name);
           casefile.setValue("description",local_evidence_list[i].description);
           casefile.setValue("image",local_evidence_list[i].image);
           casefile.endGroup();
      }
      casefile.sync();
      append_server_chatmessage("CLIENT", tr("Succesfully saved, edit doc and cmdoc link on the ini!"), "1");
      ui_ooc_chat_message->clear();
      return;

  }

  QStringList packet_contents;
  packet_contents.append(ui_ooc_chat_name->text());
  packet_contents.append(ooc_message);

  AOPacket *f_packet = new AOPacket("CT", packet_contents);

  if (server_ooc)
    ao_app->send_server_packet(f_packet);
  else
    ao_app->send_ms_packet(f_packet);

  ui_ooc_chat_message->clear();

  ui_ooc_chat_message->setFocus();
}

void Courtroom::on_ooc_toggle_clicked()
{
  if (server_ooc)
  {
    ui_ms_chatlog->show();
    ui_server_chatlog->hide();
    ui_ooc_toggle->setText(tr("Master"));

    server_ooc = false;
  }
  else
  {
    ui_ms_chatlog->hide();
    ui_server_chatlog->show();
    ui_ooc_toggle->setText(tr("Server"));

    server_ooc = true;
  }
}

void Courtroom::on_music_search_edited(QString p_text)
{
  //preventing compiler warnings
  p_text += "a";
  list_music();
  list_areas();
}

void Courtroom::on_pos_dropdown_changed(int p_index)
{
  ui_ic_chat_message->setFocus();

  if (p_index < 0 || p_index > 5)
    return;

  toggle_judge_buttons(false);

  QString f_pos;

  switch (p_index)
  {
  case 0:
    f_pos = "wit";
    break;
  case 1:
    f_pos = "def";
    break;
  case 2:
    f_pos = "pro";
    break;
  case 3:
    f_pos = "jud";
    toggle_judge_buttons(true);
    break;
  case 4:
    f_pos = "hld";
    break;
  case 5:
    f_pos = "hlp";
    break;
  default:
    f_pos = "";
  }

  if (f_pos == "" || ui_ooc_chat_name->text() == "")
    return;

  ao_app->send_server_packet(new AOPacket("CT#" + ui_ooc_chat_name->text() + "#/pos " + f_pos + "#%"));
}

void Courtroom::set_iniswap_dropdown()
{
  ui_iniswap_dropdown->clear();
  if (m_cid == -1)
  {
    ui_iniswap_dropdown->hide();
    ui_iniswap_remove->hide();
    return;
  }
  QStringList iniswaps = ao_app->get_list_file(ao_app->get_character_path(char_list.at(m_cid).name, "iniswaps.ini"));
  iniswaps.prepend(char_list.at(m_cid).name);
  if (iniswaps.size() <= 0)
  {
    ui_iniswap_dropdown->hide();
    ui_iniswap_remove->hide();
    return;
  }
  ui_iniswap_dropdown->show();
  ui_iniswap_dropdown->addItems(iniswaps);

  for (int i = 0; i < iniswaps.size(); ++i)
  {
    if (iniswaps.at(i) == current_char)
    {
      ui_iniswap_dropdown->setCurrentIndex(i);
      if (i != 0)
        ui_iniswap_remove->show();
      else
        ui_iniswap_remove->hide();
      break;
    }
  }
}

void Courtroom::on_iniswap_dropdown_changed(int p_index)
{
  ui_ic_chat_message->setFocus();
  QString iniswap = ui_iniswap_dropdown->itemText(p_index);
  ao_app->set_char_ini(char_list.at(m_cid).name, iniswap, "name", "Options");

  QStringList swaplist;
  for (int i = 0; i < ui_iniswap_dropdown->count(); ++i)
  {
    QString entry = ui_iniswap_dropdown->itemText(i);
    if (!swaplist.contains(entry) && entry != char_list.at(m_cid).name)
      swaplist.append(entry);
  }
  ao_app->write_to_file(swaplist.join("\n"), ao_app->get_character_path(char_list.at(m_cid).name, "iniswaps.ini"));
  ui_iniswap_dropdown->setCurrentIndex(p_index);
  update_character(m_cid);
  if (p_index != 0)
    ui_iniswap_remove->show();
  else
    ui_iniswap_remove->hide();
}

void Courtroom::on_iniswap_remove_clicked()
{
  if (ui_iniswap_dropdown->itemText(ui_iniswap_dropdown->currentIndex()) != char_list.at(m_cid).name)
  {
    ui_iniswap_dropdown->removeItem(ui_iniswap_dropdown->currentIndex());
    on_iniswap_dropdown_changed(0); //Reset back to original
    update_character(m_cid);
  }
}

void Courtroom::set_sfx_dropdown()
{
  ui_sfx_dropdown->clear();
  if (m_cid == -1)
  {
    ui_sfx_dropdown->hide();
    ui_sfx_remove->hide();
    return;
  }
  QStringList soundlist = ao_app->get_list_file(ao_app->get_character_path(current_char, "soundlist.ini"));

  if (soundlist.size() <= 0)
  {
    soundlist = ao_app->get_list_file(ao_app->get_theme_path("character_soundlist.ini"));
    if (soundlist.size() <= 0)
    {
      soundlist = ao_app->get_list_file(ao_app->get_default_theme_path("character_soundlist.ini"));
    }
  }

  if (soundlist.size() <= 0)
  {
    ui_sfx_dropdown->hide();
    ui_sfx_remove->hide();
    return;
  }
  soundlist.prepend("Default");

  ui_sfx_dropdown->show();
  ui_sfx_dropdown->addItems(soundlist);
  ui_sfx_dropdown->setCurrentIndex(0);
  ui_sfx_remove->hide();
}

void Courtroom::on_sfx_dropdown_changed(int p_index)
{
  ui_ic_chat_message->setFocus();

  QStringList soundlist;
  for (int i = 0; i < ui_sfx_dropdown->count(); ++i)
  {
    QString entry = ui_sfx_dropdown->itemText(i);
    if (!soundlist.contains(entry) && entry != "Default")
      soundlist.append(entry);
  }

  QStringList defaultlist = ao_app->get_list_file(ao_app->get_theme_path("character_soundlist.ini"));
  if (defaultlist.size() <= 0)
  {
    defaultlist = ao_app->get_list_file(ao_app->get_default_theme_path("character_soundlist.ini"));
  }

  if (defaultlist.size() > 0 && defaultlist.toSet().subtract(soundlist.toSet()).size() > 0) //There's a difference from the default configuration
    ao_app->write_to_file(soundlist.join("\n"), ao_app->get_character_path(current_char, "soundlist.ini")); //Create a new sound list

  ui_sfx_dropdown->setCurrentIndex(p_index);
  if (p_index != 0)
    ui_sfx_remove->show();
  else
    ui_sfx_remove->hide();
}

void Courtroom::on_sfx_remove_clicked()
{
  if (ui_sfx_dropdown->itemText(ui_sfx_dropdown->currentIndex()) != "Default")
  {
    ui_sfx_dropdown->removeItem(ui_sfx_dropdown->currentIndex());
    on_sfx_dropdown_changed(0); //Reset back to original
  }
}

void Courtroom::set_effects_dropdown()
{
  ui_effects_dropdown->clear();
  if (m_cid == -1)
  {
    ui_effects_dropdown->hide();
    return;
  }
  QStringList effectslist = ao_app->get_effects(current_char);

  if (effectslist.size() <= 0)
  {
    ui_effects_dropdown->hide();
    return;
  }


  effectslist.prepend("None");

  ui_effects_dropdown->show();
  ui_effects_dropdown->addItems(effectslist);

  //ICON-MAKING HELL
  QString p_effect = ao_app->read_char_ini(current_char, "effects", "Options");
  QString custom_path = ao_app->get_base_path() + "misc/" + p_effect + "/icons/";
  QString theme_path = ao_app->get_theme_path("effects/icons/");
  QString default_path = ao_app->get_default_theme_path("effects/icons/");
  for (int i = 0; i < ui_effects_dropdown->count(); ++i)
  {
    QString entry = ui_effects_dropdown->itemText(i);
    QString iconpath = ao_app->get_static_image_suffix(custom_path + entry);
    if (!file_exists(iconpath))
    {
      iconpath = ao_app->get_static_image_suffix(theme_path  + entry);
      if (!file_exists(iconpath))
      {
        iconpath = ao_app->get_static_image_suffix(default_path + entry);
        if (!file_exists(iconpath))
          continue;
      }
    }
    ui_effects_dropdown->setItemIcon(i, QIcon(iconpath));
  }

  ui_effects_dropdown->setCurrentIndex(0);
}

void Courtroom::on_effects_dropdown_changed(int p_index)
{
  effect = ui_effects_dropdown->itemText(p_index);
  ui_ic_chat_message->setFocus();
}

bool Courtroom::effects_dropdown_find_and_set(QString effect)
{
  for (int i = 0; i < ui_effects_dropdown->count(); ++i)
  {
    QString entry = ui_effects_dropdown->itemText(i);
    if (entry == effect)
    {
      ui_effects_dropdown->setCurrentIndex(i);
      return true;
    }
  }
  return false;
}

QString Courtroom::get_char_sfx()
{
  QString sfx = ui_sfx_dropdown->itemText(ui_sfx_dropdown->currentIndex());
  if (sfx != "" && sfx != "Default")
    return sfx;
  return ao_app->get_sfx_name(current_char, current_emote);
}

int Courtroom::get_char_sfx_delay()
{
//  QString sfx = ui_sfx_dropdown->itemText(ui_sfx_dropdown->currentIndex());
//  if (sfx != "" && sfx != "Default")
//    return 0; //todo: a way to define this
  return ao_app->get_sfx_delay(current_char, current_emote);
}

void Courtroom::on_mute_list_clicked(QModelIndex p_index)
{
  QListWidgetItem *f_item = ui_mute_list->item(p_index.row());
  QString f_char = f_item->text();
  QString real_char;

  if (f_char.endsWith(" [x]"))
    real_char = f_char.left(f_char.size() - 4);
  else
    real_char = f_char;

  int f_cid = -1;

  for (int n_char = 0 ; n_char < char_list.size() ; n_char++)
  {
    if (char_list.at(n_char).name == real_char)
      f_cid = n_char;
  }

  if (f_cid < 0 || f_cid >= char_list.size())
  {
    qDebug() << "W: " << real_char << " not present in char_list";
    return;
  }

  if (mute_map.value(f_cid))
  {
    mute_map.insert(f_cid, false);
    f_item->setText(real_char);
  }
  else
  {
    mute_map.insert(f_cid, true);
    f_item->setText(real_char + " [x]");
  }
}

void Courtroom::on_pair_list_clicked(QModelIndex p_index)
{
  QListWidgetItem *f_item = ui_pair_list->item(p_index.row());
  QString f_char = f_item->text();
  QString real_char;
  int f_cid = -1;

  if (f_char.endsWith(" [x]"))
  {
    real_char = f_char.left(f_char.size() - 4);
    f_item->setText(real_char);
  }
  else
  {
   real_char = f_char;
   for (int n_char = 0 ; n_char < char_list.size() ; n_char++)
   {
    if (char_list.at(n_char).name == real_char)
      f_cid = n_char;
   }
  }




  if (f_cid < -2 || f_cid >= char_list.size())
  {
    qDebug() << "W: " << real_char << " not present in char_list";
    return;
  }

  other_charid = f_cid;

  // Redo the character list.
  QStringList sorted_pair_list;

  for (char_type i_char : char_list)
    sorted_pair_list.append(i_char.name);

  sorted_pair_list.sort();

  for (int i = 0; i < ui_pair_list->count(); i++) {
    ui_pair_list->item(i)->setText(sorted_pair_list.at(i));
  }
  if(other_charid != -1)
  {
   f_item->setText(real_char + " [x]");
  }
}

void Courtroom::on_music_list_double_clicked(QTreeWidgetItem *p_item, int column)
{
  if (is_muted)
    return;

  QString p_song = p_item->text(column);

  if (!ui_ic_chat_name->text().isEmpty() && ao_app->cccc_ic_support_enabled)
  {
    ao_app->send_server_packet(new AOPacket("MC#" + p_song + "#" + QString::number(m_cid) + "#" + ui_ic_chat_name->text() + "#%"), false);
  }
  else
  {
    ao_app->send_server_packet(new AOPacket("MC#" + p_song + "#" + QString::number(m_cid) + "#%"), false);
  }
}

void Courtroom::on_area_list_double_clicked(QModelIndex p_model)
{
    QString p_area = area_list.at(area_row_to_number.at(p_model.row()));
    ao_app->send_server_packet(new AOPacket("MC#" + p_area + "#" + QString::number(m_cid) + "#%"), false);
}

void Courtroom::on_hold_it_clicked()
{
  if (objection_state == 1)
  {
    ui_hold_it->set_image("holdit");
    objection_state = 0;
  }
  else
  {
    ui_objection->set_image("objection");
    ui_take_that->set_image("takethat");
    ui_custom_objection->set_image("custom");

    ui_hold_it->set_image("holdit_selected");
    objection_state = 1;
  }

  ui_ic_chat_message->setFocus();
}

void Courtroom::on_objection_clicked()
{
  if (objection_state == 2)
  {
    ui_objection->set_image("objection");
    objection_state = 0;
  }
  else
  {
    ui_hold_it->set_image("holdit");
    ui_take_that->set_image("takethat");
    ui_custom_objection->set_image("custom");

    ui_objection->set_image("objection_selected");
    objection_state = 2;
  }

  ui_ic_chat_message->setFocus();
}

void Courtroom::on_take_that_clicked()
{
  if (objection_state == 3)
  {
    ui_take_that->set_image("takethat");
    objection_state = 0;
  }
  else
  {
    ui_objection->set_image("objection");
    ui_hold_it->set_image("holdit");
    ui_custom_objection->set_image("custom");

    ui_take_that->set_image("takethat_selected");
    objection_state = 3;
  }

  ui_ic_chat_message->setFocus();
}

void Courtroom::on_custom_objection_clicked()
{
  if (objection_state == 4)
  {
    ui_custom_objection->set_image("custom");
    objection_state = 0;
  }
  else
  {
    ui_objection->set_image("objection");
    ui_take_that->set_image("takethat");
    ui_hold_it->set_image("holdit");

    ui_custom_objection->set_image("custom_selected");
    objection_state = 4;
  }

  ui_ic_chat_message->setFocus();
}

void Courtroom::on_realization_clicked()
{
  if (realization_state == 0)
  {
    realization_state = 1;
    if (effects_dropdown_find_and_set("realization"))
      on_effects_dropdown_changed(ui_effects_dropdown->currentIndex());

    ui_realization->set_image("realization_pressed");
  }
  else
  {
    realization_state = 0;
    ui_effects_dropdown->setCurrentIndex(0);
    on_effects_dropdown_changed(ui_effects_dropdown->currentIndex());
    ui_realization->set_image("realization");
  }

  ui_ic_chat_message->setFocus();
}

void Courtroom::on_screenshake_clicked()
{
  if (screenshake_state == 0)
  {
    screenshake_state = 1;
    ui_screenshake->set_image("screenshake_pressed");
  }
  else
  {
    screenshake_state = 0;
    ui_screenshake->set_image("screenshake");
  }

  ui_ic_chat_message->setFocus();
}

void Courtroom::on_mute_clicked()
{
  if (ui_mute_list->isHidden())
  {
    ui_mute_list->show();
    ui_pair_list->hide();
    ui_pair_offset_spinbox->hide();
    ui_pair_button->set_image("pair_button");
    ui_mute->set_image("mute_pressed");
  }
  else
  {
    ui_mute_list->hide();
    ui_mute->set_image("mute");
  }
}

void Courtroom::on_pair_clicked()
{
  if (ui_pair_list->isHidden())
  {
    ui_pair_list->show();
    ui_pair_offset_spinbox->show();
    ui_mute_list->hide();
    ui_mute->set_image("mute");
    ui_pair_button->set_image("pair_button_pressed");
  }
  else
  {
    ui_pair_list->hide();
    ui_pair_offset_spinbox->hide();
    ui_pair_button->set_image("pair_button");
  }
}

void Courtroom::on_defense_minus_clicked()
{
  int f_state = defense_bar_state - 1;

  if (f_state >= 0)
    ao_app->send_server_packet(new AOPacket("HP#1#" + QString::number(f_state) + "#%"));
}

void Courtroom::on_defense_plus_clicked()
{
  int f_state = defense_bar_state + 1;

  if (f_state <= 10)
    ao_app->send_server_packet(new AOPacket("HP#1#" + QString::number(f_state) + "#%"));
}

void Courtroom::on_prosecution_minus_clicked()
{
  int f_state = prosecution_bar_state - 1;

  if (f_state >= 0)
    ao_app->send_server_packet(new AOPacket("HP#2#" + QString::number(f_state) + "#%"));
}

void Courtroom::on_prosecution_plus_clicked()
{
  int f_state = prosecution_bar_state + 1;

  if (f_state <= 10)
    ao_app->send_server_packet(new AOPacket("HP#2#" + QString::number(f_state) + "#%"));
}

void Courtroom::set_text_color_dropdown()
{
  //Clear the lists
  ui_text_color->clear();
  color_row_to_number.clear();

  //Clear the stored optimization information
  color_rgb_list.clear();
  color_markdown_start_list.clear();
  color_markdown_end_list.clear();
  color_markdown_remove_list.clear();
  color_markdown_talking_list.clear();

  //Update markdown colors. TODO: make a loading function that only loads the config file once instead of several times
  for (int c = 0; c < max_colors; ++c)
  {
    color_rgb_list.append(ao_app->get_chat_color(QString::number(c), current_char));
    color_markdown_start_list.append(ao_app->get_chat_markdown("c" + QString::number(c) + "_start", current_char));
    color_markdown_end_list.append(ao_app->get_chat_markdown("c" + QString::number(c) + "_end", current_char));
    color_markdown_remove_list.append(ao_app->get_chat_markdown("c" + QString::number(c) + "_remove", current_char) == "1");
    color_markdown_talking_list.append(ao_app->get_chat_markdown("c" + QString::number(c) + "_talking", current_char) == "1");

    QString color_name = ao_app->get_chat_markdown("c" + QString::number(c) + "_name", current_char);
    if (color_name.isEmpty()) //Not defined
    {
      if (c > 0)
        continue;
      color_name = tr("Default");
    }
    ui_text_color->addItem(color_name);
    color_row_to_number.append(c);
  }
}

void Courtroom::on_text_color_changed(int p_color)
{
  if (ui_ic_chat_message->selectionStart() != -1) //We have a selection!
  {
    int c = color_row_to_number.at(p_color);
    QString markdown_start = color_markdown_start_list.at(c);
    if (markdown_start.isEmpty())
    {
      qDebug() << "W: Color list dropdown selected a non-existent markdown start character";
      return;
    }
    QString markdown_end = color_markdown_end_list.at(c);
    if (markdown_end.isEmpty())
      markdown_end = markdown_start;
    int start = ui_ic_chat_message->selectionStart();
    int end = ui_ic_chat_message->selectionEnd()+1;
    ui_ic_chat_message->setCursorPosition(start);
    ui_ic_chat_message->insert(markdown_start);
    ui_ic_chat_message->setCursorPosition(end);
    ui_ic_chat_message->insert(markdown_end);
//    ui_ic_chat_message->end(false);
    ui_text_color->setCurrentIndex(0);
  }
  else
  {
    if (p_color != -1 && p_color < color_row_to_number.size())
      text_color = color_row_to_number.at(p_color);
    else
      text_color = 0;
  }
  ui_ic_chat_message->setFocus();
}

void Courtroom::on_music_slider_moved(int p_value)
{
  music_player->set_volume(p_value, 0); //Set volume on music layer
  ui_ic_chat_message->setFocus();
}

void Courtroom::on_sfx_slider_moved(int p_value)
{
  sfx_player->set_volume(p_value);
  //Set the ambience and other misc. music layers
  for (int i = 1; i < music_player->m_channelmax; ++i)
  {
    music_player->set_volume(p_value, i);
  }
  objection_player->set_volume(p_value);
  ui_ic_chat_message->setFocus();
}

void Courtroom::on_blip_slider_moved(int p_value)
{
  blip_player->set_volume(p_value);
  ui_ic_chat_message->setFocus();
}

void Courtroom::on_log_limit_changed(int value)
{
  log_maximum_blocks = value;
}

void Courtroom::on_pair_offset_changed(int value)
{
  offset_with_pair = value;
}

void Courtroom::on_witness_testimony_clicked()
{
  if (is_muted)
    return;

  ao_app->send_server_packet(new AOPacket("RT#testimony1#%"));

  ui_ic_chat_message->setFocus();
}

void Courtroom::on_cross_examination_clicked()
{
  if (is_muted)
    return;

  ao_app->send_server_packet(new AOPacket("RT#testimony2#%"));

  ui_ic_chat_message->setFocus();
}

void Courtroom::on_not_guilty_clicked()
{
  if (is_muted)
    return;

  ao_app->send_server_packet(new AOPacket("RT#judgeruling#0#%"));

  ui_ic_chat_message->setFocus();
}

void Courtroom::on_guilty_clicked()
{
  if (is_muted)
    return;

  ao_app->send_server_packet(new AOPacket("RT#judgeruling#1#%"));

  ui_ic_chat_message->setFocus();
}

void Courtroom::on_change_character_clicked()
{
  music_player->set_volume(0);
  sfx_player->set_volume(0);
  blip_player->set_volume(0);

  set_char_select();

  ui_char_select_background->show();
  ui_spectator->hide();
}

void Courtroom::on_reload_theme_clicked()
{
  ao_app->reload_theme();

  enter_courtroom();
  update_character(m_cid);

  anim_state = 4;
  text_state = 3;

  //to update status on the background
  set_background(current_background);
}

void Courtroom::on_back_to_lobby_clicked()
{
  ao_app->construct_lobby();
  ao_app->destruct_courtroom();
}

void Courtroom::on_char_select_left_clicked()
{
  --current_char_page;
  set_char_select_page();
}

void Courtroom::on_char_select_right_clicked()
{
  ++current_char_page;
  set_char_select_page();
}

void Courtroom::on_spectator_clicked()
{
  update_character(-1);
}

void Courtroom::on_call_mod_clicked()
{
  if (ao_app->modcall_reason_enabled) {
    QMessageBox errorBox;
    QInputDialog input;

    input.setWindowFlags(Qt::WindowSystemMenuHint);
    input.setLabelText(tr("Reason:"));
    input.setWindowTitle(tr("Call Moderator"));
    auto code = input.exec();

    if (code != QDialog::Accepted)
      return;

    QString text = input.textValue();
    if (text.isEmpty()) {
      errorBox.critical(nullptr, tr("Error"), tr("You must provide a reason."));
      return;
    } else if (text.length() > 256) {
      errorBox.critical(nullptr, tr("Error"), tr("The message is too long."));
      return;
    }

    QStringList mod_reason;
    mod_reason.append(text);

    ao_app->send_server_packet(new AOPacket("ZZ", mod_reason));
  } else {
    ao_app->send_server_packet(new AOPacket("ZZ#%"));
  }

  ui_ic_chat_message->setFocus();
}

void Courtroom::on_settings_clicked()
{
    ao_app->call_settings_menu();
}

void Courtroom::on_announce_casing_clicked()
{
    ao_app->call_announce_menu(this);
}

void Courtroom::on_pre_clicked()
{
  ui_ic_chat_message->setFocus();
}

void Courtroom::on_flip_clicked()
{
  ui_ic_chat_message->setFocus();
}

void Courtroom::on_additive_clicked()
{
  if (ui_additive->isChecked())
  {
    ui_ic_chat_message->home(false); //move cursor to the start of the message
    ui_ic_chat_message->insert(" "); //preface the message by whitespace
    ui_ic_chat_message->end(false); //move cursor to the end of the message without selecting anything
  }
  ui_ic_chat_message->setFocus();
}

void Courtroom::on_guard_clicked()
{
  ui_ic_chat_message->setFocus();
}

void Courtroom::on_showname_enable_clicked()
{
  ui_ic_chatlog->clear();
  first_message_sent = false;

  foreach (chatlogpiece item, ic_chatlog_history) {
      if (ui_showname_enable->isChecked())
      {
         if (item.is_song())
           append_ic_text(item.get_message(), item.get_showname(), true);
         else
           append_ic_text(item.get_message(), item.get_showname());
      }
      else
      {
          if (item.is_song())
            append_ic_text(item.get_message(), item.get_name(), true);
          else
            append_ic_text(item.get_message(), item.get_name());
      }
    }

  ui_ic_chat_message->setFocus();
}

void Courtroom::on_evidence_button_clicked()
{
  if (ui_evidence->isHidden())
  {
    ui_evidence->show();
    ui_evidence_overlay->hide();
  }
  else
  {
    ui_evidence->hide();
  }
}

void Courtroom::on_switch_area_music_clicked()
{
    if (ui_area_list->isHidden())
    {
        ui_area_list->show();
        ui_music_list->hide();
    }
    else
    {
        ui_area_list->hide();
        ui_music_list->show();
    }
}

void Courtroom::ping_server()
{
  ao_app->send_server_packet(new AOPacket("CH#" + QString::number(m_cid) + "#%"));
}

void Courtroom::on_casing_clicked()
{
  if (ao_app->casing_alerts_enabled)
  {
    if (ui_casing->isChecked())
    {
      QStringList f_packet;

      f_packet.append(ao_app->get_casing_can_host_cases());
      f_packet.append(QString::number(ao_app->get_casing_cm_enabled()));
      f_packet.append(QString::number(ao_app->get_casing_defence_enabled()));
      f_packet.append(QString::number(ao_app->get_casing_prosecution_enabled()));
      f_packet.append(QString::number(ao_app->get_casing_judge_enabled()));
      f_packet.append(QString::number(ao_app->get_casing_juror_enabled()));
      f_packet.append(QString::number(ao_app->get_casing_steno_enabled()));

      ao_app->send_server_packet(new AOPacket("SETCASE", f_packet));
    }
    else
      ao_app->send_server_packet(new AOPacket("SETCASE#\"\"#0#0#0#0#0#0#%"));
  }
}

void Courtroom::announce_case(QString title, bool def, bool pro, bool jud, bool jur, bool steno)
{
  if (ao_app->casing_alerts_enabled)
  {
    QStringList f_packet;

    f_packet.append(title);
    f_packet.append(QString::number(def));
    f_packet.append(QString::number(pro));
    f_packet.append(QString::number(jud));
    f_packet.append(QString::number(jur));
    f_packet.append(QString::number(steno));

    ao_app->send_server_packet(new AOPacket("CASEA", f_packet));
    }
}

Courtroom::~Courtroom()
{
  delete music_player;
  delete sfx_player;
  delete objection_player;
  delete blip_player;
}


#if (defined (_WIN32) || defined (_WIN64))
void Courtroom::load_bass_opus_plugin()
{
  #ifdef BASSAUDIO
  BASS_PluginLoad("bassopus.dll", 0);
  #endif
}
#elif (defined (LINUX) || defined (__linux__))
void Courtroom::load_bass_opus_plugin()
{
  #ifdef BASSAUDIO
  BASS_PluginLoad("libbassopus.so", 0);
  #endif
}
#elif defined __APPLE__
void Courtroom::load_bass_opus_plugin()
{
  QString libpath = ao_app->get_base_path() + "../../Frameworks/libbassopus.dylib";
  QByteArray ba = libpath.toLocal8Bit();
  #ifdef BASSAUDIO
  BASS_PluginLoad(ba.data(), 0);
  #endif
}
#else
#error This operating system is unsupported for bass plugins.
#endif
