<?php

/**
 * ContentNavigation
 */
class ContentNavigation {

  /**
   * instance of ContentNavigation.
   * @var ContentNavigation
   */
  protected static $instance = NULL;

  /**
   * Behavior for the level of the navigation.
   * @var LevelBehaviorBase
   */
  protected $levelBehavior = NULL;

  /**
   * The current page for the navigation
   * @var CurrentPage
   */
  protected $currentPage = NULL;

  /**
   * Source Menu tree
   * @var Array
   */
  protected $sourceTree = array();

  /**
   * getInstance().
   */
  public static function getInstance() {

    if (!self::$instance) {
      self::$instance = new ContentNavigation();
    }

    return self::$instance;

  }

  /**
   * Constructor.
   */
  private function __construct() {
  }

  /**
   * Sets the level behavior for the navigation
   * @param LevelBehaviorBase $levelBehavior
   */
  public function setLevelBehavior(LevelBehaviorBase $levelBehavior) {
    $this->levelBehavior = $levelBehavior;
  }


  /**
   * Gets the CurrentPage for the navigation
   */
  public function getCurrentPage() {
    return $this->currentPage;
  }

  /**
   * Sets the CurrentPage for the navigation
   * @param CurrentPage $currentPage
   * @throws InvalidSwiftFrameworkPageException
   */
  public function setCurrentPage(CurrentPage $currentPage) {
    $this->currentPage = $currentPage;

    if (!$this->currentPage->isFrameworkPage()) {
      throw new InvalidSwiftFrameworkPageException('The current pge is not a Swift Framework page.' . $this->currentPage->getContentType());
    }

  }

  /**
   * Sets the menu source tree.
   * @param Array $sourceTree
   */
  public function setMenuTree($sourceTree) {
    $this->sourceTree = $sourceTree;
  }

  /**
   * Build the navigation structure.
   */
  public function buildMenu() {

    if (!isset($this->levelBehavior)) {
      return $this->sourceTree;
    }
    else {
      return $this->levelBehavior->build();
    }

  }

}
