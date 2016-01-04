<?php

/**
 * LevelBehaviorBase
 */
abstract class LevelBehaviorBase {

  /**
   * Menu Tree.
   * can be partial, i.e. below the active path
   * @var Array
   */
  protected $menuTree = array();

  /**
   * Full Menu Tree.
   * @var Array
   */
  protected $menu = NULL;

  /**
   * Constructor.
   */
  public function __construct($menuTree) {
    $this->menuTree = $menuTree;
  }

  /**
   * Sets the full menu
   * @param type $menu
   */
  public function setFullMenu($menu) {
    $this->menu = $menu;
  }

  protected function getOuterLevel($tree) {
    foreach ($tree as $key => $subtree) {
      if (is_numeric($key) && !empty($subtree['#below'])) {
        $tree[$key]['#below'] = array();
      }
    }
    return $tree;
  }

  protected function getOuterTwoLevels($tree) {
    foreach ($tree as $key1 => $level1) {
      if (is_numeric($key1) && !empty($level1['#below'])) {
        foreach ($level1['#below'] as $key2 => $level2) {
          if (is_numeric($key2) && !empty($level2['#below'])) {
            $tree[$key1]['#below'][$key2]['#below'] = array();
          }
        }
      }
    }
    return $tree;
  }

  /**
   * Builds the menu for the menu Tree.
   */
  abstract public function build();

}
