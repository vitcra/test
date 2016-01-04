<?php

/**
 * CurrentPage
 */
class CurrentPage {

  /**
   * Drupal Menu item
   * @var stdClass
   */
  protected $menuItem = NULL;

  /**
   * Drupal Menu Link
   * @var stdClass
   */
  protected $menuLinkPreferred = NULL;

  /**
   * Drupal node
   * @var stdClass
   */
  protected $content = NULL;

  /**
   * Drupal node type
   * @var String
   */
  protected $contentType = '';

  /**
   * Swift page type
   * @var String
   */
  protected $pageType = '';

  /**
   * Indicates whether the current page has a topic context
   * @var Boolean
   */
  protected $hasTopicContext = FALSE;

  /**
   * The topic links (tabs) for this page
   * @var array
   */
  protected $topicLinks = array();

  /**
   * The current topic for this page
   * @var array
   */
  protected $currentTopic = NULL;

  /**
   * The current topic menu for this page
   * @var array
   */
  protected $currentTopicMenu = NULL;

  /**
   * Drupal content types available as master page type.
   * @var Array
   */
  public $masterTypes = array();

  /**
   * Drupal content types available as topic type.
   * @var Array
   */
  public $topicTypes = array();

  /**
   * Drupal content types available as topic child content type.
   * E.G. "under" a topic means: nested as menu link in the topic menu OR
   * as part of the topic content type (reference) OR as component in the
   * complete page (panels).
   * @var Array
   */
  public $topicChildTypes = array();

  /**
   * Constructor
   * Initialize a valid page request object.
   *
   * @param stdClass Menu item
   * @param stdClass Content item
   */
  public function __construct($menuItem, $menuLinkPreferred, $content = NULL) {

    $this->masterTypes = variable_get('swift_master_types', $this->masterTypes);
    $this->topicTypes = variable_get('swift_topic_types', $this->topicTypes);
    $this->topicChildTypes = variable_get('swift_topic_child_types', $this->topicChildTypes);

    $this->menuItem = $menuItem;
    $this->menuLinkPreferred = $menuLinkPreferred;

    if (isset($content)) {

      $this->content = $content;
      $this->contentType = $content->type;

      if (!empty($this->content->type)) {
        $this->pageType = $this->content->type;
      }
      else {
        $this->pageType = PAGE_TYPE_DEFAULT;
      }

      $this->initializeMenus();

      $this->setGlobalLanguage();
    }
    else {
      $this->pageType = PAGE_TYPE_DEFAULT;
      $this->initializeMenus();
    }

  }

  /**
   * Indicates whether the current page is a swift framework page.
   * @return Boolean
   */
  public function isFrameworkPage() {
    if (empty($this->pageType)) {
      return FALSE;
    }
    return TRUE;
  }

  /**
   * Indicates whether the current page is a Master page.
   * @return Boolean
   */
  public function isMasterPage() {
    return in_array($this->pageType, $this->masterTypes, TRUE);
  }

  /**
   * Indicates whether the current page is a Topic page.
   * @return Boolean
   */
  public function isTopicPage() {
    return in_array($this->pageType, $this->topicTypes, TRUE);
  }

  /**
   * Indicates whether the current page is an Orphan page.
   * @return Boolean
   *
   * TODO: this is not used and should be removed
   */
  public function isOrphanPage() {
    return $this->pageType === PAGE_TYPE_DEFAULT;
  }

  /**
   * Indicates whether the current page is a Topic content page.
   * @return Boolean
   */
  public function isTopicChildPage() {
    return in_array($this->pageType, $this->topicChildTypes, TRUE);
  }

  /**
   * Indicates whether the current page is in a Topic menu.
   *
   * see @isTopicChildPage - this one assume child pages are only nodes
   *
   * @return Boolean
   */
  public function isTopicMenuPage() {

    if (empty($this->menuItem)) {
      return FALSE;
    }

    // first try the original path
    // see for example the special events/past panel page path

    $topic_menu_item = db_select('swift_menu_links', 'sml')->fields('sml')->condition('link_path', implode('/', $this->menuItem['original_map']))->execute()->fetchAssoc();

    if ($topic_menu_item) {
      return $topic_menu_item;
    }

    $topic_menu_item = db_select('swift_menu_links', 'sml')->fields('sml')->condition('link_path', $this->menuItem['href'])->execute()->fetchAssoc();
    return $topic_menu_item;
  }

  /**
   * Indicates whether the current page has topics.
   * @return Boolean
   */
  public function hasTopicContext() {
    return $this->hasTopicContext;
  }

  /**
   * Get the page type
   * @return String
   */
  public function getPageType() {
    return $this->pageType;
  }

  /**
   * Get the content type
   * @return String
   */
  public function getContentType() {
    return $this->contentType;
  }

  /**
   * Get the menu item
   * @return stdClass
   */
  public function getMenuItem() {
    return $this->menuItem;
  }

  /**
   * Get the menu item path
   * @return stdClass
   */
  public function getMenuPath() {
    return $this->menuItem['href'];
  }

  /**
   * Get the preferred menu link
   * @return stdClass
   */
  public function getMenuLinkPreferred() {
    return $this->menuLinkPreferred;
  }

  /**
   * Get the content
   * @return stdClass
   */
  public function getContent() {
    return $this->content;
  }

  /**
   * The current topic in case of topic or topic child page.
   * @return stdClass
   */
  public function getCurrentTopic($entity_wrapped = FALSE) {
    return $entity_wrapped ? entity_metadata_wrapper('node', $this->currentTopic) : $this->currentTopic;
  }

  /**
   * The current topic menu in case of topic or topic child page.
   * @return stdClass
   */
  public function getCurrentTopicMenu() {
    return $this->currentTopicMenu;
  }

  /**
   * Get the menu links for the topic.
   * @param type $links
   */
  public function getTopicLinks() {
    return $this->topicLinks;
  }

  /**
   * Detects and initializes the menus.
   * After this method, the current page is aware of the context like
   * isTopicPage, isTopicChildPage, topic menu, ...
   */
  private function initializeMenus() {

    // Detect the position in the swift topic menu tree.
    if ($this->isTopicPage()) {

      $this->currentTopic = $this->content;

      if (!empty($this->content->tnid)) {

        menu_tree_set_path(MASTER_MENU, 'node/' . $this->content->tnid);
        $preferred_links = &drupal_static('menu_link_get_preferred');
        $this->menuLinkPreferred = menu_link_get_preferred('node/' . $this->content->tnid, MASTER_MENU);
        $preferred_links[$_GET['q']][MENU_PREFERRED_LINK] = $this->menuLinkPreferred;
        $menu = $this->getMenuLinks($this->currentTopic->tnid);
      }
      else {
        $menu = $this->getMenuLinks($this->currentTopic->nid);
      }

      $menu_links = array_shift($menu);

      if (count($menu_links) > 0) {
        $this->setTopicLinks($menu_links);
      }

      $this->hasTopicContext = TRUE;

    }
    elseif ($this->isTopicChildPage()) {

      // get the topic menu that has this child page
      $topic_menu = swift_framework_topic_menu_from_node($this->content);

      if (!empty($topic_menu->menu_name)) {
        $this->currentTopicMenu = $topic_menu;
        $this->hasTopicContext = TRUE;

        if (!empty($_GET['tl'])) {
          $topic_language = $_GET['tl'];
        }
        else {
          $topic_language = $this->content->language;
        }

        $topic_translation_nid = _swift_framework_get_translation($topic_menu->nid, $topic_language);
        $this->currentTopic = node_load($topic_translation_nid);

        // Implement our own menu_set_active_item
        $this->_setActiveItem();
      }

    }
    else if ($topic_menu_item = $this->isTopicMenuPage()) {
      $topic_menu = _swift_framework_load_topic_menu($topic_menu_item['menu_name']);
      $topic_menu->child_link = $topic_menu_item;
      $this->currentTopicMenu = $topic_menu;

      $this->hasTopicContext = TRUE;
      $this->currentTopic = node_load($topic_menu->nid);

      // Implement our own menu_set_active_item
      $this->_setActiveItem();
    }
  }

  /**
   * Set the menu links (tabs) for the topic.
   * @param type $links
   */
  private function setTopicLinks($links) {
    $default_language = language_default('language');
    // translate those that have translation
    foreach ($links as $link) {
      $nid = str_replace('node/', '', $link->link_path);
      if (is_numeric($nid)) {
        $translation_nid = _swift_framework_get_translation($nid, $this->currentTopic->language);
        if ($translation_nid != $nid) {
          $link->link_path = 'node/'. $translation_nid;
          if ($title = _swift_framework_get_tab_title($translation_nid)) {
            $link->link_title = $title;
          }
        }

        if ($translation_nid == $nid && $this->currentTopic->language != $default_language && $this->currentTopic->language != LANGUAGE_NONE) {
          $link->query = array('tl' => $this->currentTopic->language);
        }
      }
    }

    $this->topicLinks = $links;
  }

  /**
   * Get the menu links for the given parameters.
   */
  private function getMenuLinks($nid = NULL, $mlid = 0) {

    $topic_query = db_select('swift_menu_links', 'ml');
    $topic_query->fields('ml');
    $topic_query->addField('sm', 'nid', 'topic_nid');
    $topic_query->innerJoin('swift_menu', 'sm', ' sm.menu_name = ml.menu_name ');
    if (isset($nid) && $nid > 0) {
      $topic_query->condition('sm.nid', $nid);
    }
    if ($mlid >= 0) {
      $topic_query->condition('ml.plid', $mlid);
    }
    $topic_query->orderBy('ml.weight', 'ASC');
    $topic_query->orderBy('ml.p1', 'ASC');
    $topic_query->orderBy('ml.p2', 'ASC');
    $topic_query->orderBy('ml.link_title', 'ASC');

    $topic_result = $topic_query->execute();

    if ($topic_result->rowCount() > 0) {

      $menu_links = array();
      foreach ($topic_result as $topic_row) {
        $menu_links[$topic_row->menu_name][$topic_row->mlid] = $topic_row;
      }

      return $menu_links;
    }

    return array();

  }

  private function setGlobalLanguage() {
    if (!empty($this->currentTopic)) {
      if (!empty($_GET['tl'])) {
        $langcode = $_GET['tl'];
      }
      else {
        $langcode = $this->currentTopic->language;
      }

      global $language;
      if ($langcode !== LANGUAGE_NONE && $langcode !== $language->language) {
        $languages = language_list();
        if (!empty($languages[$langcode])) {
          $language = $languages[$langcode];
        }
      }
    }
  }

  /**
   * Returns the menu name of the custom menu the topic is used in
   *
   * @return mixed
   */
  public function getTopicMenuName() {
    $nid = $this->currentTopic->nid;
    if ($this->currentTopic->tnid !== "0") {
      $nid = $this->currentTopic->tnid;
    }
    $query = db_query("SELECT menu_name FROM {menu_links} WHERE link_path = :path", array(':path' => 'node/' . $nid))->fetch();
    if (!$query) {
      return false;
    }

    return $query->menu_name;
  }

  private function _setActiveItem() {
    $topic_menu = $this->currentTopicMenu;
    $menu_name = $this->getTopicMenuName($topic_menu->nid);
    if ($menu_name) {
      menu_tree_set_path($menu_name, 'node/' . $topic_menu->nid);
      $preferred_links = &drupal_static('menu_link_get_preferred');
      $this->menuLinkPreferred = menu_link_get_preferred('node/' . $topic_menu->nid, $menu_name);
      $preferred_links[$_GET['q']][MENU_PREFERRED_LINK] = $this->menuLinkPreferred;
    }

    $topic_menu_links = $this->getMenuLinks($topic_menu->nid);
    $topic_menu_links = array_pop($topic_menu_links);

    if (count($topic_menu_links) > 0) {
      $this->setTopicLinks($topic_menu_links);
    }
  }

}
